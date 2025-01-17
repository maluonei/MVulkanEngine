[[vk::binding(0, 0)]]cbuffer Constants : register(b0)
{  
    int u_previousLevel; 
    uint2 u_previousLevelDimensions; 
    uint padding0;
}

[[vk::binding(1, 0)]] Texture2D<float> BaseDepthTexture : register(t0); 
[[vk::binding(2, 0)]] RWTexture2D<float> DepthTexture[13] : register(u0); 

[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    uint2 texCoord = threadID.xy;
    if(u_previousLevel==-1){
        DepthTexture[0][texCoord] = BaseDepthTexture.Load(int3(texCoord.xy, 0)).r;
        return;
    }

    if(threadID.x>=u_previousLevelDimensions.x/2 || threadID.y>=u_previousLevelDimensions.y/2)
        return;

    uint2 previousLayerTexCoord = texCoord * 2;

    float depth0 = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord), 0)).r;
    float depth1 = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord + uint2(1, 0)), 0)).r;
    float depth2 = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord + uint2(0, 1)), 0)).r;
    float depth3 = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord + uint2(1, 1)), 0)).r;

    float minDepth = min(min(depth0, depth1), min(depth2, depth3));

    bool shouldIncludeExtraColumnFromPreviousLevel = ((u_previousLevelDimensions.x & 1) != 0);
	bool shouldIncludeExtraRowFromPreviousLevel = ((u_previousLevelDimensions.y & 1) != 0);

    if (shouldIncludeExtraColumnFromPreviousLevel) {
		float2 extraColumnTexelValues;

		extraColumnTexelValues.x = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord) + int2(2, 0), 0)).r;
		extraColumnTexelValues.y = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord) + int2(2, 1), 0)).r;

		// In the case where the width and height are both odd, need to include the
        // 'corner' value as well.
		if (shouldIncludeExtraRowFromPreviousLevel) {
			float cornerTexelValue = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord) + int2(2, 2), 0)).r;
			minDepth = min(minDepth, cornerTexelValue);
		}
		minDepth = min(minDepth, min(extraColumnTexelValues.x, extraColumnTexelValues.y));
	}
	if (shouldIncludeExtraRowFromPreviousLevel) {
		float2 extraColumnTexelValues;

		extraColumnTexelValues.x = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord) + int2(0, 2), 0)).r;
		extraColumnTexelValues.y = DepthTexture[u_previousLevel].Load(int3(int2(previousLayerTexCoord) + int2(1, 2), 0)).r;

        minDepth = min(minDepth, min(extraColumnTexelValues.x, extraColumnTexelValues.y));
	}

    DepthTexture[u_previousLevel+1][texCoord] = minDepth;
}