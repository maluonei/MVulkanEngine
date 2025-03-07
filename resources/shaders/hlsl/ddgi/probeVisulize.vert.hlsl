// HLSL Shader
struct ModelBuffer
{
    float4x4 Model;
};

struct VPBuffer{
    float4x4 View;
    float4x4 Projection;
};

//struct UniformBuffer0{
//    ModelBuffer models[512];
//};

struct UniformBuffer0{
    VPBuffer vps;
};

//[[vk::binding(0, 0)]]
//cbuffer ubo0 : register(b0)
//{
//    UniformBuffer0 ubo0;
//};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    UniformBuffer0 ubo0;
};

[[vk::binding(2, 0)]]StructuredBuffer<ModelBuffer> Models : register(t1);

struct VSInput
{
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal : NORMAL;
    [[vk::location(2)]] float2 Coord : TEXCOORD0;
    uint InstanceID : SV_InstanceID;
};

struct VSOutput
{
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;
};

float3x3 inverse(float3x3 m)
{
    float det =
        m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
        m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
        m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
        
    if (abs(det) < 1e-6)
    {
        // �������ʽ�ӽ��㣬���󲻿���
        return (float3x3) 0;
    }
        
    float invDet = 1.0 / det;

    float3x3 inv;
    inv[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
    inv[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * invDet;
    inv[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;

    inv[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * invDet;
    inv[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
    inv[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * invDet;

    inv[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
    inv[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * invDet;
    inv[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;

    return inv;
}

VSOutput main(VSInput input)
{
    VSOutput output;

    // Pass through texture coordinates
    output.texCoord = input.Coord;

    // Compute world space position
    float4 worldSpacePosition = mul(Models[input.InstanceID].Model, float4(input.Position, 1.0));

    // Calculate world position (divide by w to handle perspective divide)
    output.worldPos = worldSpacePosition.xyz / worldSpacePosition.w;

    // Compute screen space position
    float4 screenSpacePosition = mul(ubo0.vps.View, worldSpacePosition);
    screenSpacePosition = mul(ubo0.vps.Projection, screenSpacePosition);
    output.position = screenSpacePosition;

    // Compute normal in world space
    float3x3 normalMatrix = transpose(inverse((float3x3)Models[input.InstanceID].Model));
    output.normal = normalize(mul(normalMatrix, input.Normal));

    // Pass instance ID
    output.instanceID = input.InstanceID;

    return output;
}