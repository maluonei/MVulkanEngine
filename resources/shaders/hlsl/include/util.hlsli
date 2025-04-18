#pragma once
#include "common.h"

float3x3 inverse(float3x3 m)
{
    float det =
        m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
        m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
        m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

    if (abs(det) < 1e-6)
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