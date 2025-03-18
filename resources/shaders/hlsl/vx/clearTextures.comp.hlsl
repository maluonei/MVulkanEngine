[[vk::binding(0, 0)]] RWTexture3D<uint4> InputTexture : register(u0);
[[vk::binding(1, 0)]] RWTexture3D<uint4> OutputTexture : register(u1);
[[vk::binding(2, 0)]] RWTexture3D<float> SDFTexture : register(u2);

[numthreads(8, 8, 8)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    //uint3 currentPixel = DispatchThreadID;

	SDFTexture[DispatchThreadID] = 0.0f;
    InputTexture[DispatchThreadID] = uint4(0, 0, 0, 0); 
	OutputTexture[DispatchThreadID] = uint4(0, 0, 0, 0); 
}
