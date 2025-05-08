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
[[vk::binding(3, 0)]] RWStructuredBuffer<IndirectDrawArgs> CulledDrawIndices : register(u2);

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
        else{   
            int3 arrays[8] = {
                int3(-1, -1, -1),
                int3(-1, -1,  1),
                int3(-1,  1, -1),
                int3(-1,  1,  1),
                int3( 1, -1, -1),
                int3( 1, -1,  1),
                int3( 1,  1, -1),
                int3( 1,  1,  1)
            };

            int _insideNum = 0;
            for(int i=0;i<8;i++){
                float3 pointPosition = boundingSphereCenter + (float3)(arrays[i]) * bound.extent;
                float dtp = distanceToPlane(frustum.planes[i], boundingSphereCenter);
                if(dtp < 0){
                    insideNum += 1;
                    break;
                }
            }

            if(_insideNum > 0){
                insideNum += 1;
            }
        }
    }

    if(insideNum == 6) return true;
    return false;
}

[numthreads(64, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const int numInstances = 8;

    for(int i=DispatchThreadID.x * numInstances;i<DispatchThreadID.x * numInstances + numInstances;i++){
        if(i>=scene.numInstances) break;
        
        if(InFrustum(i)){
            CulledDrawIndices[i] = DrawIndices[i];
        }
        else{
            CulledDrawIndices[i].indexCount = 0;
        }
    }
} 