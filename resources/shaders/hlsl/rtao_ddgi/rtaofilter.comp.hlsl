
#define BLOCK_SIZE 16
static const int c_radius = 5;
static const int c_paddedPixelWidth = BLOCK_SIZE + c_radius * 2; //16+2*5 = 26
static const int c_paddedPixelCount = c_paddedPixelWidth * c_paddedPixelWidth; //26 * 26

[[vk::binding(0, 0)]]Texture2D<float2> gBufferDepth : register(t0);
[[vk::binding(1, 0)]]Texture2D<float2> AORaw : register(t1);
[[vk::binding(2, 0)]]RWTexture2D<float2> AOFiltered : register(u0);
[[vk::binding(6, 0)]]SamplerState linearSampler : register(s0);

groupshared float2 DistanceAndAO[c_paddedPixelWidth][c_paddedPixelWidth];

float FilterAO(int2 paddedPixelPos){

}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)] 
void main(uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID)
{
    
}