// HLSL Shader
//struct TexBuffer
//{
//    int diffuseTextureIdx;
//    int metallicAndRoughnessTextureIdx;
//    int matId;
//    int normalTextureIdx;
//
//    float3 diffuseColor;
//    int padding0;
//};
//    
//[[vk::binding(1, 0)]]
//cbuffer ubo1 : register(b1)
//{
//    TexBuffer ubo1[512];
//};
//
//[[vk::binding(2, 0)]] Texture2D textures[1024] : register(t2);
//[[vk::binding(3, 0)]] SamplerState linearSampler : register(s2);

//[[vk::binding(2, 0)]] RWTexture3D<float4> voxelTexture : register(u0);
[[vk::binding(2, 0)]] RWTexture3D<uint4> voxelTexture : register(u0);

struct PSInput
{
    float3 normal : NORMAL;
    //float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 voxPos : TEXCOORD1;
    //uint instanceID : INSTANCE_ID;
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

PSOutput main(PSInput input)
{
    PSOutput output;

    //uint3 voxelCoord = uint3(input.position.xy - float2(0.5f, 0.5f), input.position.z * ubo1.volumeResolution.z);
    //voxelTexture[int3(input.voxPos)] = float4(input.voxPos / float3(32, 32, 32), 1.f);
    //voxelTexture[int3(input.voxPos)] = float4(0.f, 0.f, 1.f, 1.f);
    //voxelTexture[int3(input.voxPos)] = float4((input.normal + float3(1.f, 1.f, 1.f)) / 2.f, 1.f);
    voxelTexture[int3(input.voxPos)] = uint4(uint3(input.voxPos), 1);

    output.Color = float4((input.normal + float3(1.f, 1.f, 1.f)) / 2.f, 1.f);

    return output;

    //float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));
//
    //int diffuseTextureIdx = ubo1[input.instanceID].diffuseTextureIdx;
    //int metallicAndRoughnessTextureIdx = ubo1[input.instanceID].metallicAndRoughnessTextureIdx;
    //int normalTextureIdx = ubo1[input.instanceID].normalTextureIdx;
//
    //// Output normal and position
    //output.Normal = float4(input.normal, input.texCoord.x);
    //output.Position = float4(input.worldPos, input.texCoord.y);
//
    //if (normalTextureIdx != -1){
    //    float3 texNormal = normalize(textures[normalTextureIdx].Sample(linearSampler, input.texCoord).xyz * 2.0 - 1.0);
    //    float3 tnorm = mul(texNormal, TBN);
    //    //output.Normal = float4(input.tangent.xyz, 1.f);
    //    //output.Normal = float4(tnorm, 1.f);
    //    output.Normal = float4(input.normal, 1.f);
    //}
//
    //// Sample diffuse texture if valid, otherwise default to white
    //if (diffuseTextureIdx != -1)
    //{
    //    output.Albedo = textures[diffuseTextureIdx].Sample(linearSampler, input.texCoord);
    //}
    //else
    //{
    //    //output.Albedo = float4(1.0, 1.0, 1.0, 1.0);
    //    output.Albedo = float4(ubo1[input.instanceID].diffuseColor, 1.0); // Use diffuse color from UBO
    //}
//
    //// Sample metallic and roughness texture if valid, otherwise default values
    //if (metallicAndRoughnessTextureIdx != -1)
    //{
    //    output.MetallicAndRoughness.rgb = textures[metallicAndRoughnessTextureIdx].Sample(linearSampler, input.texCoord).rgb;
    //}
    //else
    //{
    //    output.MetallicAndRoughness.rgb = float3(0.0, 0.5, 0.5);
    //}
    //output.MetallicAndRoughness.a = 0.0; // Optional alpha channel
//
    //// Output material ID
    //output.MatId = uint4(ubo1[input.instanceID].matId, 0, 0, 0);
//
    //return output;
}