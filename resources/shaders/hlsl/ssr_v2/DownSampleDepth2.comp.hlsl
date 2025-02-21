[[vk::binding(0, 0)]]cbuffer Constants : register(b0)
{  
    uint4 u_previousLevelDimensions[13];
}

[[vk::binding(1, 0)]] RWBuffer<float> AutomicBuffer[13] : register(u0); 
[[vk::binding(2, 0)]] Texture2D<float> BaseDepthTexture : register(t0); 
[[vk::binding(3, 0)]] RWTexture2D<float> DepthTexture[13] : register(u0); 

void CopyMip0(uint2 _threadId){
    DepthTexture[0][_threadId] = BaseDepthTexture.Load(int3(_threadId, 0)).r;
}

void DownSampleDepth(uint2 _threadId, uint currentMipLevel){
    uint previousMipLevel = currentMipLevel - 1;

    uint2 previousLevelDimensions = u_previousLevelDimensions[previousMipLevel].xy;
    uint2 texCoord = _threadId.xy;

    if(_threadId.x>=previousLevelDimensions.x/2 || _threadId.y>=previousLevelDimensions.y/2)
        return;

    uint2 previousLayerTexCoord = texCoord * 2;

    float depth0 = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord), 0)).r;
    float depth1 = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord + uint2(1, 0)), 0)).r;
    float depth2 = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord + uint2(0, 1)), 0)).r;
    float depth3 = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord + uint2(1, 1)), 0)).r;

    float minDepth = min(min(depth0, depth1), min(depth2, depth3));

    bool shouldIncludeExtraColumnFromPreviousLevel = ((previousLevelDimensions.x & 1) != 0);
	bool shouldIncludeExtraRowFromPreviousLevel = ((previousLevelDimensions.y & 1) != 0);

    if (shouldIncludeExtraColumnFromPreviousLevel) {
		float2 extraColumnTexelValues;

		extraColumnTexelValues.x = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord) + int2(2, 0), 0)).r;
		extraColumnTexelValues.y = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord) + int2(2, 1), 0)).r;

		// In the case where the width and height are both odd, need to include the
        // 'corner' value as well.
		if (shouldIncludeExtraRowFromPreviousLevel) {
			float cornerTexelValue = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord) + int2(2, 2), 0)).r;
			minDepth = min(minDepth, cornerTexelValue);
		}
		minDepth = min(minDepth, min(extraColumnTexelValues.x, extraColumnTexelValues.y));
	}
	if (shouldIncludeExtraRowFromPreviousLevel) {
		float2 extraColumnTexelValues;

		extraColumnTexelValues.x = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord) + int2(0, 2), 0)).r;
		extraColumnTexelValues.y = DepthTexture[previousMipLevel].Load(int3(int2(previousLayerTexCoord) + int2(1, 2), 0)).r;

        minDepth = min(minDepth, min(extraColumnTexelValues.x, extraColumnTexelValues.y));
	}

    //DepthTexture[currentMipLevel][texCoord] = minDepth;
    DepthTexture[currentMipLevel][texCoord] = 1.f;
}

[numthreads(1024, 1024, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    uint2 texCoord = threadID.xy;
    uint u_maxMipLevel = u_previousLevelDimensions[0].w;

    CopyMip0(texCoord);
    //GroupMemoryBarrierWithGroupSync();

    uint2 currentTexCoord = texCoord;
    for(int i=1; i<u_maxMipLevel;i++){
        if(currentTexCoord.x/2==0 && currentTexCoord.y/2==0){
            currentTexCoord = uint2(texCoord.x/2, texCoord.y/2);
            DownSampleDepth(currentTexCoord, i);
        }
        //GroupMemoryBarrierWithGroupSync();
    }
}