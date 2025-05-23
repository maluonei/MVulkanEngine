//#pragma once
#ifndef UTIL_HLSLI
#define UTIL_HLSLI

#include "common.h"

float3x3 inverse(float3x3 m)
{
    float det =
        m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
        m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
        m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

    if (abs(det) < 1e-9)
    {
        // �������ʽ�ӽ��㣬���󲻿���
        return (float3x3)0;
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

float4x4 inverse(float4x4 m)
{
    float4x4 inv;

    inv[0][0] =  m[1][1]*m[2][2]*m[3][3] - m[1][1]*m[2][3]*m[3][2] - m[2][1]*m[1][2]*m[3][3]
               + m[2][1]*m[1][3]*m[3][2] + m[3][1]*m[1][2]*m[2][3] - m[3][1]*m[1][3]*m[2][2];

    inv[0][1] = -m[0][1]*m[2][2]*m[3][3] + m[0][1]*m[2][3]*m[3][2] + m[2][1]*m[0][2]*m[3][3]
               - m[2][1]*m[0][3]*m[3][2] - m[3][1]*m[0][2]*m[2][3] + m[3][1]*m[0][3]*m[2][2];

    inv[0][2] =  m[0][1]*m[1][2]*m[3][3] - m[0][1]*m[1][3]*m[3][2] - m[1][1]*m[0][2]*m[3][3]
               + m[1][1]*m[0][3]*m[3][2] + m[3][1]*m[0][2]*m[1][3] - m[3][1]*m[0][3]*m[1][2];

    inv[0][3] = -m[0][1]*m[1][2]*m[2][3] + m[0][1]*m[1][3]*m[2][2] + m[1][1]*m[0][2]*m[2][3]
               - m[1][1]*m[0][3]*m[2][2] - m[2][1]*m[0][2]*m[1][3] + m[2][1]*m[0][3]*m[1][2];

    inv[1][0] = -m[1][0]*m[2][2]*m[3][3] + m[1][0]*m[2][3]*m[3][2] + m[2][0]*m[1][2]*m[3][3]
               - m[2][0]*m[1][3]*m[3][2] - m[3][0]*m[1][2]*m[2][3] + m[3][0]*m[1][3]*m[2][2];

    inv[1][1] =  m[0][0]*m[2][2]*m[3][3] - m[0][0]*m[2][3]*m[3][2] - m[2][0]*m[0][2]*m[3][3]
               + m[2][0]*m[0][3]*m[3][2] + m[3][0]*m[0][2]*m[2][3] - m[3][0]*m[0][3]*m[2][2];

    inv[1][2] = -m[0][0]*m[1][2]*m[3][3] + m[0][0]*m[1][3]*m[3][2] + m[1][0]*m[0][2]*m[3][3]
               - m[1][0]*m[0][3]*m[3][2] - m[3][0]*m[0][2]*m[1][3] + m[3][0]*m[0][3]*m[1][2];

    inv[1][3] =  m[0][0]*m[1][2]*m[2][3] - m[0][0]*m[1][3]*m[2][2] - m[1][0]*m[0][2]*m[2][3]
               + m[1][0]*m[0][3]*m[2][2] + m[2][0]*m[0][2]*m[1][3] - m[2][0]*m[0][3]*m[1][2];

    inv[2][0] =  m[1][0]*m[2][1]*m[3][3] - m[1][0]*m[2][3]*m[3][1] - m[2][0]*m[1][1]*m[3][3]
               + m[2][0]*m[1][3]*m[3][1] + m[3][0]*m[1][1]*m[2][3] - m[3][0]*m[1][3]*m[2][1];

    inv[2][1] = -m[0][0]*m[2][1]*m[3][3] + m[0][0]*m[2][3]*m[3][1] + m[2][0]*m[0][1]*m[3][3]
               - m[2][0]*m[0][3]*m[3][1] - m[3][0]*m[0][1]*m[2][3] + m[3][0]*m[0][3]*m[2][1];

    inv[2][2] =  m[0][0]*m[1][1]*m[3][3] - m[0][0]*m[1][3]*m[3][1] - m[1][0]*m[0][1]*m[3][3]
               + m[1][0]*m[0][3]*m[3][1] + m[3][0]*m[0][1]*m[1][3] - m[3][0]*m[0][3]*m[1][1];

    inv[2][3] = -m[0][0]*m[1][1]*m[2][3] + m[0][0]*m[1][3]*m[2][1] + m[1][0]*m[0][1]*m[2][3]
               - m[1][0]*m[0][3]*m[2][1] - m[2][0]*m[0][1]*m[1][3] + m[2][0]*m[0][3]*m[1][1];

    inv[3][0] = -m[1][0]*m[2][1]*m[3][2] + m[1][0]*m[2][2]*m[3][1] + m[2][0]*m[1][1]*m[3][2]
               - m[2][0]*m[1][2]*m[3][1] - m[3][0]*m[1][1]*m[2][2] + m[3][0]*m[1][2]*m[2][1];

    inv[3][1] =  m[0][0]*m[2][1]*m[3][2] - m[0][0]*m[2][2]*m[3][1] - m[2][0]*m[0][1]*m[3][2]
               + m[2][0]*m[0][2]*m[3][1] + m[3][0]*m[0][1]*m[2][2] - m[3][0]*m[0][2]*m[2][1];

    inv[3][2] = -m[0][0]*m[1][1]*m[3][2] + m[0][0]*m[1][2]*m[3][1] + m[1][0]*m[0][1]*m[3][2]
               - m[1][0]*m[0][2]*m[3][1] - m[3][0]*m[0][1]*m[1][2] + m[3][0]*m[0][2]*m[1][1];

    inv[3][3] =  m[0][0]*m[1][1]*m[2][2] - m[0][0]*m[1][2]*m[2][1] - m[1][0]*m[0][1]*m[2][2]
               + m[1][0]*m[0][2]*m[2][1] + m[2][0]*m[0][1]*m[1][2] - m[2][0]*m[0][2]*m[1][1];

    float det = m[0][0]*inv[0][0] + m[0][1]*inv[1][0] + m[0][2]*inv[2][0] + m[0][3]*inv[3][0];

    if (abs(det) < 1e-12)
        return float4x4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0); // 不可逆，返回零矩阵或自定义值

    inv = inv / det;
    return inv;
}


float4x4 GetInvViewProjMat(VPBuffer vp){
    float4x4 view = vp.View;
    float4x4 proj = vp.Projection;

    float4x4 viewProj = mul(proj, view);
    float4x4 invViewProj = inverse(viewProj);

    //float4x4 invView = inverse(view);
    //float4x4 invProj = inverse(proj);

    return invViewProj;
}

float3 GetPixelDirection(
    MCameraBuffer cam,
    MScreenBuffer screen,
    float2 uv)
{
    float3 UP = float3(0.f, 1.f, 0.f);
    float aspectRatio = (float)screen.WindowRes.x / (float)screen.WindowRes.y;

    float3 forward = normalize(cam.cameraDir);
    float3 right = normalize(cross(forward, UP));
    float3 up = normalize(cross(right, forward));

    float halfHeight = tan(cam.fovY * 0.5f) * cam.zNear;
    float halfWidth = aspectRatio * halfHeight;

    float3 zPlaneCenter = cam.zNear * forward + cam.cameraPos;
    float3 zPlaneCorner = zPlaneCenter - right * halfWidth + up * halfHeight;

    //float2 pixelOffset = 0.5f / float2(ubo1.ScreenResolution);
    float3 pixelPos = zPlaneCorner + right * uv.x * 2 * halfWidth - up * uv.y * 2 * halfHeight;

    return normalize(pixelPos - cam.cameraPos);
}



float2 EncodeNormalOctahedron(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    float2 enc = n.xy;
    if (n.z < 0.0f)
    {
        enc = (1.0f - abs(enc.yx)) * float2(enc.x >= 0.0f ? 1.0f : -1.0f, enc.y >= 0.0f ? 1.0f : -1.0f);
    }
    return enc;
}

float3 DecodeNormalOctahedron(float2 enc)
{
    float3 n = float3(enc.xy, 1.0f - abs(enc.x) - abs(enc.y));
    if (n.z < 0.0f)
    {
        n.xy = (1.0f - abs(n.yx)) * float2(n.x >= 0.0f ? 1.0f : -1.0f, n.y >= 0.0f ? 1.0f : -1.0f);
    }
    return normalize(n);
}



void PackGbuffer(
    float3 normal,
    float3 position,
    float2 uv,
    float3 albedo,
    float3 metallicAndRoughness,
    float3 motionVector,
    uint instanceID,
    out uint4 gBuffer0,
    out uint4 gBuffer1
){
    float roughness = metallicAndRoughness.g;
    float metallic = metallicAndRoughness.b;

    float2 octa =  EncodeNormalOctahedron(normal);
    uint2 packedOcta = f32tof16(octa);
    //uint3 packedNormal = f32tof16(normal);
    uint3 packedPosition = f32tof16(position);
    uint2 packedUv = f32tof16(uv);
    uint3 packedAlbedo = f32tof16(albedo);
    uint packedMetallic = f32tof16(metallic);
    uint packedRoughness = f32tof16(roughness);
    uint3 packedMotionVector = f32tof16(motionVector);

    gBuffer0.x = ((packedOcta.x & 0xFFFF) << 16) | (packedOcta.y & 0xFFFF);
    //gBuffer0.y = ((packedNormal.z & 0xFFFF) << 16) | (packedPosition.x & 0xFFFF);
    gBuffer0.y = ((instanceID & 0xFFFF) << 16) | (packedPosition.x & 0xFFFF);
    gBuffer0.z = ((packedPosition.y & 0xFFFF) << 16) | (packedPosition.z & 0xFFFF);
    gBuffer0.w = ((packedUv.x & 0xFFFF) << 16) | (packedUv.y & 0xFFFF);

    gBuffer1.x = ((packedAlbedo.x & 0xFFFF) << 16) | (packedAlbedo.y & 0xFFFF);
    gBuffer1.y = ((packedAlbedo.z & 0xFFFF) << 16) | (packedMetallic & 0xFFFF);
    gBuffer1.z = ((packedRoughness & 0xFFFF) << 16) | (packedMotionVector.x & 0xFFFF);
    gBuffer1.w = ((packedMotionVector.y & 0xFFFF) << 16) | (packedMotionVector.z & 0xFFFF);
}


void UnpackGbuffer(
    uint4 gBuffer0,
    uint4 gBuffer1,
    out float3 normal,
    out float3 position,
    out float2 uv,
    out float3 albedo,
    out float metallic,
    out float roughness,
    out float3 motionVector,
    out uint instanceID
){
    //uint2 unpackedNormal = uint2((gBuffer0.x & 0xFFFF0000) >> 16, gBuffer0.x & 0xFFFF);
    uint2 unpackedOcta = uint2((gBuffer0.x & 0xFFFF0000) >> 16, gBuffer0.x & 0xFFFF);
    uint3 unpackedPosition = uint3(gBuffer0.y & 0xFFFF, (gBuffer0.z & 0xFFFF0000) >> 16, gBuffer0.z & 0xFFFF);
    uint2 unpackedUV = uint2((gBuffer0.w & 0xFFFF0000) >> 16, gBuffer0.w & 0xFFFF);

    uint3 unpackedAlbedo = uint3((gBuffer1.x & 0xFFFF0000) >> 16, gBuffer1.x & 0xFFFF, (gBuffer1.y & 0xFFFF0000) >> 16);
    uint unpackedMetallic = gBuffer1.y & 0xFFFF;
    uint unpackedRoughness = (gBuffer1.z & 0xFFFF0000) >> 16;
    uint3 unpackedMotionVector = uint3(gBuffer1.z & 0xFFFF, (gBuffer1.w & 0xFFFF0000) >> 16, gBuffer1.w & 0xFFFF);

    float2 octa = f16tof32(unpackedOcta);
    normal = DecodeNormalOctahedron(octa);
    //normal.xy = f16tof32(unpackedNormal);
    //normal.z = sqrt(1.f - dot(normal.xy, normal.xy));
    position = f16tof32(unpackedPosition);
    uv = f16tof32(unpackedUV);
    albedo = f16tof32(unpackedAlbedo);
    metallic = f16tof32(unpackedMetallic);
    roughness = f16tof32(unpackedRoughness);
    motionVector = f16tof32(unpackedMotionVector);
    instanceID = (gBuffer0.y & 0xFFFF0000) >> 16;
}


#endif