struct UniformBuffer0
{
    float3 aabbMin;
    float padding;
    float3 aabbMax;
    uint padding2;

    uint3 gridResolution;
	uint stepSize;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    UniformBuffer0 ubo0;
};

[[vk::binding(1, 0)]] RWTexture3D<uint4> InputTexture : register(u0);
[[vk::binding(2, 0)]] RWTexture3D<uint4> OutputTexture : register(u1);
[[vk::binding(3, 0)]] RWTexture3D<float> SDFTexture : register(u2);

[numthreads(8, 8, 8)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint3 currentPixel = DispatchThreadID;

    float bestDistance = 999999999999.;
	uint4 bestColor = uint4(0, 0, 0, 0); 

	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			for (int k = -1; k <= 1; k++) {
				uint3 neighbourCoord = currentPixel + (ubo0.stepSize * uint3(i, j, k));

				if (any(neighbourCoord < uint3(0, 0, 0)) || any(neighbourCoord >= uint3(ubo0.gridResolution.xyz))) {
					continue;
				}

				uint4 seed = InputTexture[neighbourCoord];
				float dist = distance(seed.xyz, currentPixel);

				if ((seed.a > 0) && (dist < bestDistance)) {
					bestDistance = dist; 
					bestColor = seed;
				}
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();
	SDFTexture[DispatchThreadID] = bestDistance;// / 10.f;
	OutputTexture[DispatchThreadID] = bestColor; 
}
