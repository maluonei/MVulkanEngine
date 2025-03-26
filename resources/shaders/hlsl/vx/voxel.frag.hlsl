// HLSL Shader
struct TexBuffer
{
    int diffuseTextureIdx;
    int metallicAndRoughnessTextureIdx;
    int matId;
    int normalTextureIdx;

    float3 diffuseColor;
    int padding0;
};
    
[[vk::binding(2, 0)]]
cbuffer ubo2 : register(b1)
{
    TexBuffer ubo2[512];
};


//[[vk::binding(2, 0)]] RWTexture3D<float4> voxelTexture : register(u0);
[[vk::binding(3, 0)]] Texture2D textures[1024] : register(t2);
[[vk::binding(4, 0)]] RWTexture3D<uint4> voxelTexture : register(u0);
[[vk::binding(5, 0)]] RWTexture3D<float4> albedoTexture : register(u1);
//[[vk::binding(6, 0)]] RWTexture3D<float4> normalTexture : register(u2);
[[vk::binding(6, 0)]] SamplerState linearSampler : register(s1);

struct PSInput
{
    float3 normal : NORMAL;
    //float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 voxPos : TEXCOORD1;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    //[[vk::location(0)]] float4 Normal : SV_TARGET0;
    //[[vk::location(1)]] float4 Position : SV_TARGET1;
    //[[vk::location(2)]] float4 Albedo : SV_TARGET2;
    //[[vk::location(3)]] float4 MetallicAndRoughness : SV_TARGET3;
    //[[vk::location(4)]] uint4 MatId : SV_TARGET4;

    [[vk::location(0)]] float4 Color : SV_TARGET0;
};

void StoreAlbedo(int3 coord, float3 albedo){
    float4 val = albedoTexture[coord];
    float3 color = val.rgb;
    float sum = val.a;

    if(sum < 409600.f){
        float3 avgAlbedo = ((color * sum) + albedo) / (sum + 1);
        albedoTexture[coord] = float4(avgAlbedo, sum + 1);
    }
}

//void StoreNormal(int3 coord, float3 normal){
//    float4 val = normalTexture[coord];
//    float3 color = val.rgb;
//    float sum = val.a;
//
//    if(sum < 409600.f){
//        float3 avgNormal = ((color * sum) + normal) / (sum + 1);
//        normalTexture[coord] = float4(normalize(avgNormal), sum + 1);
//    }
//}

PSOutput main(PSInput input)
{
    PSOutput output;

    voxelTexture[int3(input.voxPos)] = uint4(uint3(input.voxPos), 1);

    output.Color = float4((input.normal + float3(1.f, 1.f, 1.f)) / 2.f, 1.f);

    float3 albedo;
    int diffuseTextureIdx = ubo2[input.instanceID].diffuseTextureIdx;
    if (diffuseTextureIdx != -1)
    {
        albedo = textures[diffuseTextureIdx].Sample(linearSampler, input.texCoord);
    }
    else
    {
        albedo = float4(ubo2[input.instanceID].diffuseColor, 1.0); // Use diffuse color from UBO
    }
    
    StoreAlbedo(int3(input.voxPos), albedo.rgb);
    //StoreNormal(int3(input.voxPos), input.normal);

    return output;
}