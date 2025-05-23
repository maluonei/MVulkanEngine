#include "common.h"
#include "util.hlsli"

[[vk::binding(0, 0)]]
cbuffer vpBuffer : register(b0)
{
    VPBuffer vp;
};

[[vk::binding(1, 0)]]
cbuffer vpBuffer_p : register(b1)
{
    VPBuffer vp_p;
};

[[vk::binding(2, 0)]]
cbuffer screenBuffer : register(b2)
{
    MScreenBuffer screenBuffer;
};

[[vk::binding(3, 0)]] Texture2D<float> depth_p : register(t0);
[[vk::binding(4, 0)]] RWTexture2D<float> depth : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 threadID: SV_DispatchThreadID) {
    uint2 texCoord = threadID.xy;

    float zPrev = depth_p.Load(int3(texCoord, 0)).r;

    if (zPrev <= 0.0f || zPrev >= 1.0f) {
        depth[texCoord] = 1.0f; // 或者别写入
        return;
    }

    float2 xy = (texCoord + float2(0.5f, 0.5f)) / float2(screenBuffer.WindowRes.x, screenBuffer.WindowRes.y);

    float4 ndcPrev = float4(xy * 2.0f - 1.0f, zPrev, 1.0f);
    //ndcPrev.y = 1.f - ndcPrev.y;
    float4x4 invViewProjPrev = GetInvViewProjMat(vp_p);
    float4 worldPos = mul(invViewProjPrev, ndcPrev);
    worldPos /= worldPos.w;

    float4 ndcCurrent = mul(vp.Projection, mul(vp.View, float4(worldPos.xyz, 1.0f)));
    float zCurrent = ndcCurrent.z / ndcCurrent.w;

    depth[texCoord] = zCurrent;
    //depth[texCoord] = zPrev;
}