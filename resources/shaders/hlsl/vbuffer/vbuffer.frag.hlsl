struct MeshOutput {
    float4 Position : SV_POSITION;
    uint4 InstanceIDAndPrimitiveIDAndMaterialID : TEXCOORD0;
    float2 Barycentrics : SV_Barycentrics; // DX12 自动提供重心坐标
    //float3 Normal   : NORMAL;
    float3 Color    : COLOR;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
    //[[vk::location(0)]] uint InstanceIDAndPrimitiveID : SV_TARGET0; // 16-bit instance ID, 16-bit primitive ID
    //[[vk::location(1)]] uint BarycentricCoordAndMaterialID : SV_TARGET1; // 16-bit barycentric coordinates, 16-bit material ID
    [[vk::location(1)]] uint4 VBuffer1 : SV_TARGET1; // 16-bit instance ID, 16-bit primitive ID
    [[vk::location(2)]] float4 VBuffer2 : SV_TARGET2; // 16-bit instance ID, 16-bit primitive ID
    //[[vk::location(1)]] uint BarycentricCoordAndMaterialID : SV_TARGET1; // 16-bit barycentric coordinates, 16-bit material ID
};

PSOutput main(MeshOutput input)
{
    PSOutput output;

    output.VBuffer1.xyz = input.InstanceIDAndPrimitiveIDAndMaterialID.xyz; // 16-bit instance ID
    output.VBuffer2.rg = input.Barycentrics; // 16-bit barycentric coordinates
    //float3 normal = normalize(input.Normal);

    output.Color = float4(input.Color, 1.0f);
    //output.Color = float4(1.0f, 0.f, 0.f, 1.0f);
    //output.Color = float4(normal * 2.f - 1.f, 1.0f);


    return output;
}