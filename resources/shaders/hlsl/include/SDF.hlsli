//#pragma once
#ifndef SDF_HLSLI
#define SDF_HLSLI

struct MDFStruct{
    RWTexture3D<float> SDFTexture;
    RWTexture3D<float4> albedoTexture;

    float3 aabbMin;
    float3 aabbMax;
    uint3 gridResolution;
};

void Swap(inout float a, inout float b) {
    float temp = a;
    a = b;
    b = temp;
}

float GetSDFValue(
    float3 position, 
    MDFStruct mdf,
    out bool inSDFTex){
    float3 offset = 0.5f * (mdf.aabbMax - mdf.aabbMin) / float3(mdf.gridResolution);
    float epsilon = offset;
    //debug = 0;

    if( position.x < (mdf.aabbMin.x - epsilon) || position.x > (mdf.aabbMax.x + epsilon) || 
        position.y < (mdf.aabbMin.y - epsilon) || position.y > (mdf.aabbMax.y + epsilon) || 
        position.z < (mdf.aabbMin.z - epsilon) || position.z > (mdf.aabbMax.z + epsilon))
    {
        inSDFTex = false;

        //float3 offset = 0.5f / float3(mdf.gridResolution);
        uint3 gridCoord = uint3(((position - mdf.aabbMin) / (mdf.aabbMax - mdf.aabbMin)) * (mdf.gridResolution));
        
        return -1.f;
    }

    inSDFTex = true;

    uint3 gridCoord = uint3(((position - mdf.aabbMin) / (mdf.aabbMax - mdf.aabbMin)) * float3(mdf.gridResolution));
    gridCoord.x = clamp(gridCoord.x, 0, mdf.gridResolution.x-1);
    gridCoord.y = clamp(gridCoord.y, 0, mdf.gridResolution.y-1);
    gridCoord.z = clamp(gridCoord.z, 0, mdf.gridResolution.z-1);
    
    //return 10.f;
    return mdf.SDFTexture.Load(gridCoord);
}

float2 GetNearestDistance(
    float3 position, float3 direction, 
    float3 aabbMin, float3 aabbMax,
    out bool hit){
    float tMin = -1e20f;
    float tMax = 1e20f;
    hit = false;

    //x axis
    if(direction.x != 0.f){
        float t1 = (aabbMin.x - position.x) / direction.x;
        float t2 = (aabbMax.x - position.x) / direction.x;

        if (t1 > t2) Swap(t1, t2);
        
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
    }

    //y axis
    if(direction.y != 0.f){
        float t1 = (aabbMin.y - position.y) / direction.y;
        float t2 = (aabbMax.y - position.y) / direction.y;

        if (t1 > t2) Swap(t1, t2);
        
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
    }

    //z axis
    if(direction.z != 0.f){
        float t1 = (aabbMin.z - position.z) / direction.z;
        float t2 = (aabbMax.z - position.z) / direction.z;

        if (t1 > t2) Swap(t1, t2);
        
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
    }

    if(tMin > tMax || (tMin <= tMax && tMax < 0)){
        return float2(-1.f, -1.f);
    }

    hit = true;
    if(tMin < 0 && tMax > 0) 
        return float2(0.f, tMax);

    return float2(tMin, tMax);
} 

float GetNextPixelSDF(
    float3 position, float3 direction, MDFStruct mdf){
    int3 currentIndex = int3(float3((position - mdf.aabbMin) / (mdf.aabbMax - mdf.aabbMin)) * (mdf.gridResolution));
    //currentIndex = clamp(currentIndex, 0, mdf.gridResolution-1);
    float3 voxelSize = (mdf.aabbMax - mdf.aabbMin) / (mdf.gridResolution);

    float3 currentAabbMin = mdf.aabbMin + currentIndex * voxelSize;
    float3 currentAabbMax = currentAabbMin + voxelSize;

    bool hit = false;
    float2 nextHitT = GetNearestDistance(position, direction, currentAabbMin, currentAabbMax, hit);
    if(hit) 
        return nextHitT.y + 2e-3 * voxelSize.x;
    else
        return 0.5f * voxelSize.x;
}

void RayMarchSDF_inv(float3 position, float3 direction, MDFStruct mdf, int maxRaymarchSteps, out float depth){
    depth = 0.f;
    int step = 0;

    float3 pos = position;
    while(step < maxRaymarchSteps){
        float nextPosSdf = GetNextPixelSDF(pos, direction, mdf);
        depth += nextPosSdf;

        pos = position + direction * depth;

        bool inSdf = false;
        float sdfValue = GetSDFValue(pos, mdf, inSdf);

        if(sdfValue!=0.f || !inSdf){
            depth *= -1;
            return;
        }

        step++;
    }
    depth *= -1;
    return;
    //float nextPosSdf = GetNextPixelSDF(pos, direction, mdf);

}

bool RayMarchSDF(float3 position, float3 direction, MDFStruct mdf, int maxRaymarchSteps, out float depth){
    float start = 0.f;
    bool hit = false;
    float2 aabbT = GetNearestDistance(position, direction, mdf.aabbMin, mdf.aabbMax, hit);
    float voxelSize = (mdf.aabbMax - mdf.aabbMin) / (mdf.gridResolution);
    depth = aabbT.x + 1e-3 * voxelSize.x;
    depth = max(0.f, depth);

    if(!hit) return false;

    int step=0;

    for(;step<maxRaymarchSteps && depth<=aabbT.y;step++){
        float3 pos = position + direction * depth;// * gridOffset;
        //struc.debuginfo0.xyz = pos + 0.2f;

        bool inSdf = false;
        //int debug = 0;
        float sdfValue = GetSDFValue(pos, mdf, inSdf);

        //struc.sdfValue = sdfValue;

        if(!inSdf){
            depth = -1000.f;
            //struc.debuginfo8 = struc.debuginfo7;
            return false;
        }

        if(sdfValue == 0.f){
            if(step == 0){
                RayMarchSDF_inv(pos, direction, mdf, maxRaymarchSteps, depth);
            }
            return true;
        }

        if(sdfValue > 0.f && sdfValue < 2.f){
            float nextPosSdf = GetNextPixelSDF(pos, direction, mdf);
            depth += nextPosSdf;
        }
        else{
            depth += (sdfValue - 1.f) * voxelSize.x;
        }
        //depth = max(0.f, depth);
    }

    return false;
} 

#endif
