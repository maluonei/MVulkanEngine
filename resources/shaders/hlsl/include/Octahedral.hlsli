#pragma once
#define PI 3.14159265359

/**
 * Returns either -1 or 1 based on the sign of the input value.
 * If the input is zero, 1 is returned.
 */
float RTXGISignNotZero(float v)
{
    return (v >= 0.f) ? 1.f : -1.f;
}

/**
 * 2-component version of RTXGISignNotZero.
 */
float2 RTXGISignNotZero(float2 v)
{
    return float2(RTXGISignNotZero(v.x), RTXGISignNotZero(v.y));
}


/**
 * Computes the normalized octahedral direction that corresponds to the
 * given normalized coordinates on the [-1, 1] square.
 * The opposite of DDGIGetOctahedralCoordinates().
 * Used by DDGIProbeBlendingCS() in ProbeBlending.hlsl.
 */
//float3 DDGIGetOctahedralDirection(float2 coords)
//{
//    float3 direction = float3(coords.x, coords.y, 1.f - abs(coords.x) - abs(coords.y));
//    if (direction.z < 0.f)
//    {
//        direction.xy = (1.f - abs(direction.yx)) * RTXGISignNotZero(direction.xy);
//    }
//    return normalize(direction);
//}
float3 DDGIGetOctahedralDirection(float2 coords)
{
    float3 direction = float3(coords.x, 1.f - abs(coords.x) - abs(coords.y), coords.y);
    if (direction.y < 0.f)
    {
        direction.xz = (1.f - abs(direction.zx)) * RTXGISignNotZero(direction.xz);
    }
    return normalize(direction);
}

/**
 * Computes the octant coordinates in the normalized [-1, 1] square, for the given a unit direction vector.
 * The opposite of DDGIGetOctahedralDirection().
 * Used by GetDDGIVolumeIrradiance() in Irradiance.hlsl.
 */
//float2 DDGIGetOctahedralCoordinates(float3 direction)
//{
//    float l1norm = abs(direction.x) + abs(direction.y) + abs(direction.z);
//    float2 uv = direction.xy * (1.f / l1norm);
//    if (direction.z < 0.f)
//    {
//        uv = (1.f - abs(uv.yx)) * RTXGISignNotZero(uv.xy);
//    }
//    return uv;
//}

float2 DDGIGetOctahedralCoordinates(float3 direction)
{
    float l1norm = abs(direction.x) + abs(direction.y) + abs(direction.z);
    float2 uv = direction.xz * (1.f / l1norm);
    if (direction.y < 0.f)
    {
        uv = (1.f - abs(uv.yx)) * RTXGISignNotZero(uv.xy);
    }
    return uv;
}