struct MeshOutput {
    float4 Position : SV_POSITION;
    uint4 InstanceIDAndMatletIDAndMaterialID : TEXCOORD0;
    float2 Barycentrics : SV_Barycentrics; // DX12 自动提供重心坐标
    uint primitiveID : SV_PrimitiveID;
    //float3 Normal   : NORMAL;
    float3 Color    : COLOR;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
    //[[vk::location(0)]] uint InstanceIDAndPrimitiveID : SV_TARGET0; // 16-bit instance ID, 16-bit primitive ID
    //[[vk::location(1)]] uint BarycentricCoordAndMaterialID : SV_TARGET1; // 16-bit barycentric coordinates, 16-bit material ID
    [[vk::location(1)]] uint4 VBuffer1 : SV_TARGET1; // 16-bit instance ID, 16-bit primitive ID
    //[[vk::location(2)]] float4 VBuffer2 : SV_TARGET2; // 16-bit instance ID, 16-bit primitive ID
    //[[vk::location(1)]] uint BarycentricCoordAndMaterialID : SV_TARGET1; // 16-bit barycentric coordinates, 16-bit material ID
};

//MeshletID:16
//PrimitiveID:16
//Barycentric Coord:32
//MaterialID:16

uint PackFloat2To32(float2 value) {
    uint2 packed = f32tof16(value); // 转换为 16-bit FP16
    return (packed.x & 0xFFFF) | ((packed.y & 0xFFFF) << 16);
}

uint4 pack(uint4 value, float2 barycentrics){
    uint x = (value.w & 0xFFFF) | ((value.y & 0xFFFF) << 16);
    uint y = (value.z & 0xFFFF);
    uint z = PackFloat2To32(barycentrics);
    //y = (y<<16) | (value.z & 0xFFFF);
    return uint4(x, y, z, 0);
}

PSOutput main(MeshOutput input)
{
    PSOutput output;

    //output.VBuffer1.xyzw = uint4(input.InstanceIDAndMatletIDAndMaterialID.xyz, input.primitiveID); // 16-bit instance ID
    //output.VBuffer2.rg = input.Barycentrics; // 16-bit barycentric coordinates
    
    output.VBuffer1.xyzw = pack(uint4(input.InstanceIDAndMatletIDAndMaterialID.xyz, input.primitiveID), input.Barycentrics);
    //float3 normal = normalize(input.Normal);

    output.Color = float4(input.Color, 1.0f);
    //output.Color = float4(1.0f, 0.f, 0.f, 1.0f);
    //output.Color = float4(normal * 2.f - 1.f, 1.0f);


    return output;
}