// HLSL Shader
struct MVPBuffer
{
    float4x4 Model;
    float4x4 View;
    float4x4 Projection;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    MVPBuffer mvp;
};

//[[vk::binding(2, 0)]] Texture2D textures[1024] : register(t2);

struct VSInput
{
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal : NORMAL;
    [[vk::location(2)]] float2 Coord : TEXCOORD0;
    [[vk::location(3)]] float3 Tangent : TEXCOORD1;
    [[vk::location(4)]] float3 Bitangent : TEXCOORD2;
    uint InstanceID : SV_InstanceID;
};

struct VSOutput
{
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;

    //float3x3 TBN : TEXCOORD1;
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
    float4 worldSpacePosition = mul(mvp.Model, float4(input.Position, 1.0));

    // Calculate world position (divide by w to handle perspective divide)
    output.worldPos = worldSpacePosition.xyz / worldSpacePosition.w;

    // Compute screen space position
    float4 screenSpacePosition = mul(mvp.View, worldSpacePosition);
    screenSpacePosition = mul(mvp.Projection, screenSpacePosition);
    output.position = screenSpacePosition;

    // Compute normal in world space
    float3x3 normalMatrix = transpose(inverse((float3x3)mvp.Model));
    output.normal = normalize(mul(normalMatrix, input.Normal));

    // Pass instance ID
    output.instanceID = input.InstanceID;

    //float3 calculatedNormal = cross(input.Tangent, input.Bitangent);
    //if(dot(calculatedNormal, input.Normal) < 0.0)
    //    input.Tangent *= -1.f;

    float4 tangent = mul(mvp.Model, float4(input.Tangent, 1.0));
    //float4 bitangent = mul(mvp.Model, float4(input.Bitangent, 1.0));
    output.tangent = normalize(tangent.xyz / tangent.w);
    output.bitangent = cross(output.normal, output.tangent);

    //float4 tangent = mul(mvp.Model, float4(input.Tangent, 1.0));
    ////output.tangent = tangent.xyz;
    //output.tangent = normalize(tangent.xyz / tangent.w);
    //output.tangent = input.Tangent;
    ////output.bitangent = normalize(mul(mvp.Model, float4(input.Bitangent, 1.0)));
    //output.bitangent = cross(output.normal, output.tangent);
    //float3 T = normalize(input.Tangent);
    //float3 B = normalize(input.Bitangent);
    //float3 N = normalize(input.Normal);
    //float3x3 TBN = float3x3(T, B, N);
    //output.TBN = TBN;
    //float3 tnorm = mul(normalize(textureNormalMap.Sample(samplerNormalMap, input.UV).xyz * 2.0 - float3(1.0, 1.0, 1.0)), TBN);

    return output;
}