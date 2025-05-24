////#define SHARED_CODE_HLSL
#include "Common.h"

[[vk::binding(0, 0)]]
cbuffer FrustumBuffer : register(b0)
{
    FrustumBuffer frustum;
};

[[vk::binding(1, 0)]]
cbuffer cullingBuffer : register(b1)
{
    MCullingBuffer culling;
};

[[vk::binding(2, 0)]]
cbuffer vpBuffer : register(b2)
{
    VPBuffer vp;
};

[[vk::binding(3, 0)]]
cbuffer hizDimensionBuffer : register(b3)
{
    HizDimensionBuffer hiz;
};

[[vk::binding(4, 0)]] StructuredBuffer<InstanceBound> InstanceBounds : register(t0);
[[vk::binding(5, 0)]] StructuredBuffer<IndirectDrawArgs> DrawIndices : register(t1);
[[vk::binding(6, 0)]] Texture2D<float> depth : register(t3);
[[vk::binding(7, 0)]] RWStructuredBuffer<IndirectDrawArgs> CulledDrawIndices : register(u0);
[[vk::binding(8, 0)]] RWStructuredBuffer<int> CulledIndirectInstances : register(u1);
[[vk::binding(9, 0)]] RWStructuredBuffer<int> NumDrawInstances : register(u2);
//[[vk::binding(10, 0)]] RWStructuredBuffer<float4> InstanceDebugDepth : register(u3);
//[[vk::binding(11, 0)]] RWStructuredBuffer<DebugBounds> InstanceBoundsDebug : register(u4);

bool IsOutsideThePlane(float4 plane, float3 pointPosition){
    if(dot(plane.xyz, pointPosition) + plane.w > 0)
        return true;
    return false;
}

float distanceToPlane(float4 plane, float3 pointPosition){
    float distance = (dot(plane.xyz, pointPosition) + plane.w) / length(plane.xyz);
    return distance;
}

bool FrustumCulling(int instanceIndex, int cullingMode){
    InstanceBound bound = InstanceBounds[instanceIndex];
    //bool inFrustum = false;
    int insideNum = 0;

    float3 boundingSphereCenter = bound.center;
    float boundingSphereRadius = length(bound.extent);

    for(int i=0;i<6;i++){
        if(cullingMode==CULLINGMODE_SPHERE){
            float dtp = distanceToPlane(frustum.planes[i], boundingSphereCenter);
            if(dtp < boundingSphereRadius){
                insideNum += 1;
            }
        }
        else if(cullingMode==CULLINGMODE_AABB){   
            float3 arrays[8] = {
                float3(-1.f, -1.f, -1.f),
                float3(-1.f, -1.f,  1.f),
                float3(-1.f,  1.f, -1.f),
                float3(-1.f,  1.f,  1.f),
                float3( 1.f, -1.f, -1.f),
                float3( 1.f, -1.f,  1.f),
                float3( 1.f,  1.f, -1.f),
                float3( 1.f,  1.f,  1.f)
            };

            int _insideNum = 0;
            for(int j=0;j<8;j++){
                float3 pointPosition = bound.center + arrays[j] * bound.extent;
                float dtp = distanceToPlane(frustum.planes[i], pointPosition);
                if(dtp < 0){
                    _insideNum += 1;
                    break;
                }
            }

            if(_insideNum > 0){
                insideNum += 1;
            }
        }
        else{
            return true;
        }
    }

    if(insideNum == 6) return true;
    return false;
}

float loadDepth(int2 uv, int mipLevel) {
    return depth.Load(int3(uv, mipLevel));
}

bool HizCulling(int instanceIndex) {
    InstanceBound bound = InstanceBounds[instanceIndex];

    float3 arrays[8] = {
        float3(-1.f, -1.f, -1.f),
        float3(-1.f, -1.f, 1.f),
        float3(-1.f, 1.f, -1.f),
        float3(-1.f, 1.f, 1.f),
        float3(1.f, -1.f, -1.f),
        float3(1.f, -1.f, 1.f),
        float3(1.f, 1.f, -1.f),
        float3(1.f, 1.f, 1.f)
    };

    float3 cornerNdcMin = float3(1.0f, 1.0f, 1.0f);
    float3 cornerNdcMax = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 8; i++) {
        float3 pointPosition = bound.center + arrays[i] * bound.extent;

        float4 screenPos;
        //screenPos = mul(vp.Projection, mul(vp.View, float4(pointPosition, 1.0)));
        screenPos = mul(mul(vp.Projection, vp.View), float4(pointPosition, 1.0));
        screenPos.xyz /= screenPos.w;
        screenPos.xy = screenPos.xy * 0.5f + 0.5f;
        screenPos.y = 1.f - screenPos.y;

        cornerNdcMin = min(cornerNdcMin, screenPos.xyz);
        cornerNdcMax = max(cornerNdcMax, screenPos.xyz);
    }
    float2 rectSize = cornerNdcMax.xy - cornerNdcMin.xy;

    if (rectSize.x >= 0.5f || rectSize.y >= 0.5f) {
        return true;
    }
    
    int2 hizRes0 = int2(hiz.hizDimensions[0].xy);
    uint numMipLevels = hiz.hizDimensions[0].z;
    //uint baseMipLevel = 0;

    float2 rectSizeInt2 = rectSize * (float2)hizRes0;
    float numRectPixels = max(rectSizeInt2.x, rectSizeInt2.y);
    // float mipLevel = ceil(log2(numRectPixels));
    uint mipLevel = (uint)clamp(floor(log2(numRectPixels)), 0.0f, (float)(numMipLevels - 1));

    float closerDepth = min(cornerNdcMin.z, cornerNdcMax.z);
    int2 hizRes = hiz.hizDimensions[mipLevel].xy;

    //float2 texCoord = 0.5f * (cornerNdcMax.xy + cornerNdcMin.xy) * (float2)hizRes;
    //float hizDepth = loadDepth((int2)texCoord, (int)mipLevel);
    //float maxDepth = hizDepth;
    int2 texCoord00 = int2(cornerNdcMin.xy * (float2)hizRes);
    int2 texCoord01 = int2(float2(cornerNdcMin.x, cornerNdcMax.y) * (float2)hizRes);
    int2 texCoord10 = int2(float2(cornerNdcMax.x, cornerNdcMin.y) * (float2)hizRes);
    int2 texCoord11 = int2(cornerNdcMax.xy * (float2)hizRes);
    float hizDepth00 = loadDepth(texCoord00, (int)mipLevel);
    float hizDepth01 = loadDepth(texCoord01, (int)mipLevel);
    float hizDepth10 = loadDepth(texCoord10, (int)mipLevel);
    float hizDepth11 = loadDepth(texCoord11, (int)mipLevel);
    float maxDepth = max(hizDepth00, max(hizDepth01, max(hizDepth10, hizDepth11)));

    return (closerDepth <= maxDepth);
}

[numthreads(1, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    IndirectDrawArgs originArgs = DrawIndices[DispatchThreadID.x];
    int numInstances = originArgs.instanceCount;
    int firstInstance = originArgs.firstInstance;

    bool doFrustumCulling = culling.doFrustumCulling > 0;
    bool doHizCulling = culling.doHizCulling > 0;

    bool doCulling = doFrustumCulling || doHizCulling;
    if (!doCulling) {
        CulledDrawIndices[DispatchThreadID.x] = DrawIndices[DispatchThreadID.x];
        return;
    }

    int unculledInstances = 0;

    if (doFrustumCulling && doHizCulling) {
        for (int i = 0; i < numInstances; i++) {
            if (FrustumCulling(firstInstance + i, culling.frustumCullingMode)) {
                if (HizCulling(firstInstance + i)) {
                    CulledIndirectInstances[firstInstance + unculledInstances] = firstInstance + i;
                    unculledInstances += 1;
                    if (culling.cullingCountCalculate) {
                        InterlockedAdd(NumDrawInstances[0], 1);
                    }
                }
            }
        }
    }
    else if (doFrustumCulling) {
        for (int i = 0; i < numInstances; i++) {
            if (FrustumCulling(firstInstance + i, culling.frustumCullingMode)) {
                CulledIndirectInstances[firstInstance + unculledInstances] = firstInstance + i;
                unculledInstances += 1;
                if (culling.cullingCountCalculate) {
                    InterlockedAdd(NumDrawInstances[0], 1);
                }
            }
        }
    }
    else if (doHizCulling) {
        for (int i = 0; i < numInstances; i++) {
            if (HizCulling(firstInstance + i)) {
                CulledIndirectInstances[firstInstance + unculledInstances] = firstInstance + i;
                unculledInstances += 1;
                if (culling.cullingCountCalculate) {
                    InterlockedAdd(NumDrawInstances[0], 1);
                }
            }
        }
    }
    else {
        if (culling.cullingCountCalculate) {
            InterlockedAdd(NumDrawInstances[0], numInstances);
        }

        CulledDrawIndices[DispatchThreadID.x] = DrawIndices[DispatchThreadID.x];
        return;
    }

    //for (int i = 0; i < numInstances; i++) {
    //    bool frustumCullingResult = false;
    //    bool hizCullingResult = false;
    //    if (doFrustumCulling)
    //    {
    //        frustumCullingResult = FrustumCulling(firstInstance + i, culling.frustumCullingMode);
    //    }
    //    if (doHizCulling) {
    //        if (frustumCullingResult)
    //            hizCullingResult = HizCulling(firstInstance + i);
    //    }
//
    //    if (frustumCullingResult && hizCullingResult) {
    //        CulledIndirectInstances[firstInstance + unculledInstances] = firstInstance + i;
    //        unculledInstances += 1;
    //        if (culling.cullingCountCalculate) {
    //            InterlockedAdd(NumDrawInstances[0], 1);
    //        }
    //    }
    //}

    for(int i=unculledInstances;i<numInstances;i++){
        CulledIndirectInstances[firstInstance + i] = -1;
    }

    CulledDrawIndices[DispatchThreadID.x] = DrawIndices[DispatchThreadID.x];
    CulledDrawIndices[DispatchThreadID.x].instanceCount = unculledInstances;
    if(unculledInstances == 0){
        CulledDrawIndices[DispatchThreadID.x].indexCount = 0;
    }

} 