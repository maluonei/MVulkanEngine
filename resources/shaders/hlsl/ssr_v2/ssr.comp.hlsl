#include "Common.h"
#include "util.hlsli"

[[vk::binding(0, 0)]]
cbuffer cameraBuffer : register(b0)
{
    MCameraBuffer cameraBuffer;
}

[[vk::binding(1, 0)]]
cbuffer vpBuffer : register(b1)
{
    VPBuffer vp;
};

[[vk::binding(2, 0)]]
cbuffer hizDimensionBuffer : register(b2)
{
    HizDimensionBuffer hiz;
};

[[vk::binding(3, 0)]]
cbuffer ssrBuffer : register(b3)
{
    SSRBuffer ssrBuffer;
};

[[vk::binding(4, 0)]]
cbuffer screenBuffer : register(b4)
{
    MScreenBuffer screenBuffer;
}

[[vk::binding(5, 0)]]Texture2D<uint4> gBuffer0 : register(t0);
[[vk::binding(6, 0)]]Texture2D<uint4> gBuffer1 : register(t1);
[[vk::binding(7, 0)]]Texture2D<float> HIZ : register(t2);
[[vk::binding(8, 0)]]Texture2D<float4> Render : register(t3);
[[vk::binding(9, 0)]]RWTexture2D<float4> SSRRender : register(u0);

[[vk::binding(10, 0)]]SamplerState linearSampler : register(s0);

#define FLOAT_MAX                          3.402823466e+38

int GetMinMipLevel(){
    return hiz.hizDimensions[0].z - 1;
}

void InitialAdvanceRay(float3 origin, float3 direction, float3 inv_direction, 
    int currentMipLevel,
    float2 floor_offset, float2 uv_offset, 
    out float3 position, out float current_t)
{
    float2 current_mip_resolution = (float2)hiz.hizDimensions[currentMipLevel].xy;
    float2 current_mip_resolution_inv = 1.0 / current_mip_resolution;

    float2 current_mip_position = current_mip_resolution * origin.xy;

    // Intersect ray with the half box that is pointing away from the ray origin.
    float2 xy_plane = floor(current_mip_position) + floor_offset;
    xy_plane = xy_plane * current_mip_resolution_inv + uv_offset;

    // o + d * t = p' => t = (p' - o) / d
    float2 t = (xy_plane - origin.xy) * inv_direction.xy;
    
    current_t = min(t.x, t.y);
    position = origin + current_t * direction;
}

bool AdvanceRay(
    float3 origin, 
    float3 direction, 
    float3 inv_direction, 
    int currentMipLevel, 
    float2 floor_offset, 
    float2 uv_offset, 
    float surface_z, 
    inout float3 position, 
    inout float current_t,
    inout bool _above_surface,
    inout bool zSame
    ) 
{
    float2 current_mip_resolution = (float2)hiz.hizDimensions[currentMipLevel].xy;
    float2 current_mip_resolution_inv = 1.0 / current_mip_resolution;

    float2 current_mip_position = current_mip_resolution * position.xy;
    
    float2 xy_plane = floor(current_mip_position) + floor_offset;
    xy_plane = xy_plane * current_mip_resolution_inv + uv_offset;

    //surface_z = surface_z - (1e-5);
    float3 boundary_planes = float3(xy_plane, surface_z);

    // Intersect ray with the half box that is pointing away from the ray origin.
    // o + d * t = p' => t = (p' - o) / d
    float3 t = (boundary_planes - origin) * inv_direction;

    t.z = direction.z > 0 ? t.z : FLOAT_MAX;

    float t_min = min(min(t.x, t.y), t.z);

    bool above_surface = surface_z > position.z;

    bool skipped_tile = (t_min != t.z) && above_surface;

    _above_surface = above_surface;
    zSame = (t_min != t.z);

    // Make sure to only advance the ray if we're still above the surface.
    current_t = above_surface ? t_min : current_t;

    // Advance ray
    position = origin + current_t * direction;

    return skipped_tile;
}

float SampleDepth(int2 uv, int mipLevel){
    //return HIZ.SampleLevel(linearSampler, float2(uv.x, 1.f-uv.y), mipLevel).r;
    //return HIZ.SampleLevel(linearSampler, uv, mipLevel).r;
    return HIZ.Load(int3(uv, mipLevel));
}

float3 SampleRender(float2 uv){
    //return Render.Sample(linearSampler, float2(uv.x, 1.f-uv.y)).rgb;
    return Render.Sample(linearSampler, uv).rgb;
}

float3 SSR_Trace(
    float3 origin, 
    float3 direction,
    uint max_traversal_intersections, 
    out bool valid_hit, 
    out float depth,
    out bool above_surface,
    out bool zSame)
{
    float3 color = float3(0.f, 0.f, 0.f);
    const float3 inv_direction = select(direction != 0, 1.f / direction, FLOAT_MAX);

    int mostDetailedMip = 0;
    float2 uv_offset = 0.005 * exp2(mostDetailedMip) / float2(screenBuffer.WindowRes.x, screenBuffer.WindowRes.y);
    uv_offset = select(direction.xy < 0, -uv_offset, uv_offset);

    // Offset applied depending on current mip resolution to move the boundary to the left/right upper/lower border depending on ray direction.
    float2 floor_offset = select(direction.xy < 0, float2(0.f, 0.f), float2(1.f, 1.f));

    int currentMipLevel = mostDetailedMip;

    float current_t = 0.f;
    float3 position;

    InitialAdvanceRay(
        origin, 
        direction, 
        inv_direction, 
        currentMipLevel,
        floor_offset, 
        uv_offset, 
        position, 
        current_t);

    uint i = 0;
    bool exit_due_to_low_occupancy = false;
    uint minMipLevel = GetMinMipLevel();

    while(i < max_traversal_intersections && currentMipLevel >= mostDetailedMip){
        float2 current_mip_resolution = (float2)hiz.hizDimensions[currentMipLevel].xy;
        float2 current_mip_position = position.xy * current_mip_resolution;
        float surface_z = SampleDepth(current_mip_position, currentMipLevel);

        //valid_hit = true;
        //return float3(surface_z, surface_z, surface_z);
        bool _above_surface = false;
        bool _zSame = false;


        bool skipped_tile = AdvanceRay(
            origin, 
            direction, 
            inv_direction, 
            currentMipLevel, 
            floor_offset, 
            uv_offset, 
            surface_z, 
            position, 
            current_t,
            _above_surface,
            _zSame);

        if(i==0){
            above_surface = _above_surface;
            zSame = _zSame;
        }

        if(position.x<0.f || position.x>1.f || position.y<0.f || position.y>1.f){
            valid_hit = false;
            return float3(0.f, 0.f, 0.f);
        }

        bool nextMipOutofRange = skipped_tile && (currentMipLevel >= minMipLevel);

        if(!nextMipOutofRange){
            currentMipLevel += skipped_tile ? 1 : -1;
        }

        ++i;
    }

    depth = i / float(max_traversal_intersections);

    valid_hit = (i <= max_traversal_intersections);
    if(position.x<0.f || position.x>1.f || position.y<0.f || position.y>1.f)
        return float3(0.f, 0.f, 0.f);
    
    return SampleRender(position.xy);
}

[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    if(threadID.x >= screenBuffer.WindowRes.x || threadID.y >= screenBuffer.WindowRes.y)
        return;

    uint3 texCoord = uint3(threadID.xy, 0);
    
    uint4 gBufferValue0 = gBuffer0.Load(texCoord);
    uint4 gBufferValue1 = gBuffer1.Load(texCoord);
    //float4 gBufferValue2 = gBuffer2.Load(coord);
    float3 fragNormal;
    float3 fragPos;
    float2 fragUV;
    float3 fragAlbedo;
    float metallic;
    float roughness;
    float3 motionVector;
    uint instanceID;

    UnpackGbuffer(
        gBufferValue0, 
        gBufferValue1, 
        fragNormal, 
        fragPos, 
        fragUV, 
        fragAlbedo, 
        metallic, 
        roughness,
        motionVector,
        instanceID);

    if(instanceID == 46){
        float3 viewDir = normalize(cameraBuffer.cameraPos - fragPos);
        float3 reflectDir = normalize(reflect(-viewDir, fragNormal));

        float3 targetPosition = fragPos + reflectDir;

        float4x4 viewProj = mul(vp.Projection, vp.View);

        float4 startFrag = mul(viewProj, float4(fragPos, 1.f));
        startFrag.xyz   /= startFrag.w;
        startFrag.xy     = startFrag.xy * 0.5 + 0.5;
        startFrag.y      = 1.f - startFrag.y;
        //startFrag.xy     = startFrag.xy * texSize;
        
        float4 endFrag   = float4(targetPosition, 1.f);
        endFrag          = mul(viewProj, endFrag);
        endFrag.xyz     /= endFrag.w;
        endFrag.xy       = endFrag.xy * 0.5 + 0.5;
        endFrag. y       = 1.f - endFrag.y;
        //endFrag.xy       = endFrag.xy * texSize;

        float3 screenDirection = normalize(endFrag.xyz - startFrag.xyz);

        bool valid_hit;
        uint max_traversal_intersections = ssrBuffer.g_max_traversal_intersections;
        float depth; 
        bool above_surface;
        bool zSame;

        float3 color = SSR_Trace(
            startFrag.xyz + 1e-3 * screenDirection, 
            screenDirection, 
            max_traversal_intersections, 
            valid_hit, 
            depth,
            above_surface,
            zSame);

        //if(above_surface && zSame){
        //    SSRRender[texCoord.xy] = float4(1.f, 0.f, 0.f, 1.f);
        //}
        //else if(above_surface){
        //    SSRRender[texCoord.xy] = float4(0.f, 1.f, 0.f, 1.f);
        //}
        //else if(zSame){
        //    SSRRender[texCoord.xy] = float4(0.f, 0.f, 1.f, 1.f);
        //}
        //else{
        //    SSRRender[texCoord.xy] = float4(1.f, 0.f, 1.f, 1.f);
        //}

        //SSRRender[texCoord.xy] = float4(depth, depth, depth, 1.f);
        if (valid_hit){
            SSRRender[texCoord.xy] = float4(color, 1.f);
            //SSRRender[texCoord.xy] = float4(1.f, 0.f, 0.f, 1.f);
        }
        else{
            SSRRender[texCoord.xy] = float4(0.f, 0.f, 0.f, 0.f);
            //SSRRender[texCoord.xy] = float4(0.f, 1.f, 0.f, 1.f);
        }
    }
    else{
        SSRRender[texCoord.xy] = float4(0.f, 0.f, 0.f, 0.f);
    }
}