////#define SHARED_CODE_HLSL
#include "Common.h"

[[vk::binding(0, 0)]]
cbuffer FrustumBuffer : register(b0)
{
    FrustumBuffer frustum;
};

[[vk::binding(4, 0)]]
cbuffer sceneBuffer : register(b1)
{
    MSceneBuffer scene;
};

[[vk::binding(1, 0)]] StructuredBuffer<InstanceBound> InstanceBounds : register(t0);
[[vk::binding(2, 0)]] StructuredBuffer<IndirectDrawArgs> DrawIndices : register(t1);
//[[vk::binding(2, 0)]] StructuredBuffer<ModelBuffer> Models : register(t1);
//[[vk::binding(2, 0)]] StructuredBuffer<ModelBuffer> DrawIndices : register(t1);

[[vk::binding(3, 0)]] RWStructuredBuffer<IndirectDrawArgs> CulledDrawIndices : register(u2);
[[vk::binding(6, 0)]] RWStructuredBuffer<int> CulledIndirectInstances : register(u4);
//[[vk::binding(2, 0)]] RWStructuredBuffer<ModelBuffer> CulledModels : register(t1);
[[vk::binding(5, 0)]] RWStructuredBuffer<int> NumDrawInstances : register(u3);

bool IsOutsideThePlane(float4 plane, float3 pointPosition){
    if(dot(plane.xyz, pointPosition) + plane.w > 0)
        return true;
    return false;
}

float distanceToPlane(float4 plane, float3 pointPosition){
    float distance = (dot(plane.xyz, pointPosition) + plane.w) / length(plane.xyz);
    return distance;
}

bool InFrustum(int instanceIndex){
    InstanceBound bound = InstanceBounds[instanceIndex];
    //bool inFrustum = false;
    int insideNum = 0;

    float3 boundingSphereCenter = bound.center;
    float boundingSphereRadius = length(bound.extent);

    for(int i=0;i<6;i++){
        if(scene.cullingMode==CULLINGMODE_SPHERE){
            float dtp = distanceToPlane(frustum.planes[i], boundingSphereCenter);
            if(dtp < boundingSphereRadius){
                insideNum += 1;
            }
        }
        else if(scene.cullingMode==CULLINGMODE_AABB){   
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

[numthreads(1, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    //const int numInstances = 8;
//
    //for(int i=DispatchThreadID.x * numInstances;i<DispatchThreadID.x * numInstances + numInstances;i++){
    //    if(i>=scene.numInstances) break;
    //    
    //    if(InFrustum(i)){
    //        CulledDrawIndices[i] = DrawIndices[i];
    //        InterlockedAdd(NumDrawInstances[0], 1);
    //    }
    //    else{
    //        CulledDrawIndices[i].indexCount = 0;
    //    }
    //}

    IndirectDrawArgs originArgs = DrawIndices[DispatchThreadID.x];
    int numInstances = originArgs.instanceCount;
    int firstInstance = originArgs.firstInstance;

    int unculledInstances = 0;
    for(int i=0;i<numInstances;i++){
        if(InFrustum(firstInstance + i)){
            CulledIndirectInstances[firstInstance + unculledInstances] = firstInstance + i;
            unculledInstances += 1;
            //InterlockedAdd(NumDrawInstances[0], 1);
        }
    }

    for(int i=unculledInstances;i<numInstances;i++){
        CulledIndirectInstances[firstInstance + i] = -1;
    }

    CulledDrawIndices[DispatchThreadID.x] = DrawIndices[DispatchThreadID.x];
    CulledDrawIndices[DispatchThreadID.x].instanceCount = unculledInstances;
    if(unculledInstances == 0){
        CulledDrawIndices[DispatchThreadID.x].indexCount = 0;
    }
    //else{
    //    //CulledDrawIndices[DispatchThreadID.x] = DrawIndices[DispatchThreadID.x];
    //    CulledDrawIndices[DispatchThreadID.x].instanceCount = unculledInstances;
    //}

} 