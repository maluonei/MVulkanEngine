#include "Common.h"
#include "util.hlsli"

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
    //float  depth : SV_Depth;
};

struct PSOutput
{
    float4 color : SV_Target0;
};


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
[[vk::binding(11, 0)]]Texture2D<float> HIZ2[13] : register(t10);
[[vk::binding(8, 0)]]Texture2D<float4> Render : register(t3);
[[vk::binding(9, 0)]]RWTexture2D<float4> SSRRender : register(u0);

[[vk::binding(10, 0)]]SamplerState linearSampler : register(s0);
//[[vk::binding(10, 0)]]SamplerState nearSampler : register(s0);

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
    
    current_t = max(min(t.x, t.y), 0.f);
    position = origin + current_t * direction;
}

bool AdvanceRay(float3 origin, float3 direction, float3 inv_direction, 
    int currentMipLevel,
    float2 floor_offset, 
    float2 uv_offset, 
    float surface_z, 
    inout float3 position, 
    inout float current_t,
    inout float3 txyz,
    inout float3 _boundary_planes) 
{
    float2 current_mip_resolution = (float2)hiz.hizDimensions[currentMipLevel].xy;
    float2 current_mip_resolution_inv = 1.0 / current_mip_resolution;

    float2 current_mip_position = current_mip_resolution * position.xy;
    
    float2 xy_plane = floor(current_mip_position) + floor_offset;
    xy_plane = xy_plane * current_mip_resolution_inv + uv_offset;
    float3 boundary_planes = float3(xy_plane, surface_z);
    _boundary_planes = boundary_planes;

    // Intersect ray with the half box that is pointing away from the ray origin.
    // o + d * t = p' => t = (p' - o) / d
    float3 t = (boundary_planes - origin) * inv_direction;

    t.z = direction.z > 0 ? t.z : FLOAT_MAX;
    txyz = t;

    float t_min = min(min(t.x, t.y), t.z);

    bool above_surface = surface_z > position.z;

    bool skipped_tile = (t_min != t.z) && above_surface;

    // Make sure to only advance the ray if we're still above the surface.
    current_t = above_surface ? t_min : current_t;

    // Advance ray
    position = origin + current_t * direction;

    return skipped_tile;
}

//float LoadDepth(float2 uv, int mipLevel){
//    //return HIZ.SampleLevel(linearSampler, float2(uv.x, 1.f-uv.y), mipLevel).r;
//    return HIZ.SampleLevel(linearSampler, uv, mipLevel).r;
//}

//float SampleDepth(int2 uv, int mipLevel){
//    //return HIZ.SampleLevel(linearSampler, float2(uv.x, 1.f-uv.y), mipLevel).r;
//    //return HIZ.SampleLevel(linearSampler, uv, mipLevel).r;
//    return HIZ.Load(int3(uv, mipLevel));
//}

float SampleDepth(int2 uv, int mipLevel){
    //return HIZ.SampleLevel(linearSampler, float2(uv.x, 1.f-uv.y), mipLevel).r;
    //return HIZ.SampleLevel(linearSampler, uv, mipLevel).r;
    return HIZ2[mipLevel].Load(int3(uv, 0));
}

float3 SampleRender(float2 uv){
    //return Render.Sample(linearSampler, float2(uv.x, 1.f-uv.y)).rgb;
    return Render.Sample(linearSampler, uv).rgb;
}

float3 SSR_Trace(float3 origin, float3 direction,
    uint max_traversal_intersections, 
    out bool valid_hit, 
    out float depth, 
    out bool initialOnSurface,
    out float3 finalPosition,
    out float _t,
    out float3 txyz,
    out float3 _boundary_planes)
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
    InitialAdvanceRay(origin,
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

    //{
    //    float surface_z = LoadDepth(position.xy, currentMipLevel);
    //    initialOnSurface = (surface_z > position.z);
    //}

    while(i < max_traversal_intersections && currentMipLevel >= mostDetailedMip){
        float2 current_mip_resolution = (float2)hiz.hizDimensions[currentMipLevel].xy;
        float2 current_mip_position = position.xy * current_mip_resolution;
        float surface_z = SampleDepth(current_mip_position, currentMipLevel);

        //valid_hit = true;
        //return float3(surface_z, surface_z, surface_z);

        bool skipped_tile = AdvanceRay(origin, 
            direction, 
            inv_direction, 
            currentMipLevel, 
            floor_offset, 
            uv_offset, 
            surface_z,
            position, 
            current_t, 
            txyz,
            _boundary_planes);

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

    finalPosition = position;
    _t = current_t;

    depth = i / float(max_traversal_intersections);// * 20;

    //valid_hit = (i <= max_traversal_intersections) && (currentMipLevel < 0);
    valid_hit = (i <= max_traversal_intersections);
    if(position.x<0.f || position.x>1.f || position.y<0.f || position.y>1.f)
        return float3(0.f, 0.f, 0.f);
    
    return SampleRender(position.xy);
}

float3 ProjectionPosition(float4x4 Proj, float3 viewSpacePos){
    float4 screenSpacePos = mul(Proj, float4(viewSpacePos, 1.f));
    screenSpacePos.xyz /= screenSpacePos.w;
    screenSpacePos.xy = screenSpacePos.xy * 0.5 + 0.5;
    screenSpacePos.y = 1.f - screenSpacePos.y;

    return screenSpacePos.xyz;
}

PSOutput main(PSInput input)
{
    PSOutput output;
    //if(threadID.x >= screenBuffer.WindowRes.x || threadID.y >= screenBuffer.WindowRes.y)
    //    return;

    uint3 texCoord = uint3(uint2(input.texCoord * float2(screenBuffer.WindowRes.x, screenBuffer.WindowRes.y)), 0);
    
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
        //worldSpace
        float3 viewDir = normalize(fragPos - cameraBuffer.cameraPos);
        float3 reflectDir = normalize(reflect(viewDir, fragNormal));

        //worldSpace
        float3 targetPosition = fragPos + reflectDir;

        float4x4 viewProj = mul(vp.Projection, vp.View);

        //float4 worldSpaceStartPos = float4(fragPos, 1.f);
        float4 viewSpaceStartPos = mul(vp.View, float4(fragPos, 1.f));
        float3 viewSpaceRayDirection = normalize(viewSpaceStartPos.xyz);
        float3 viewSpaceNormal = normalize(mul(vp.View, float4(fragNormal, 0.f)).xyz);
        float3 viewSpaceReflectDirection = reflect(viewSpaceRayDirection, viewSpaceNormal);
        float3 screenSpaceStartPosition = ProjectionPosition(vp.Projection, viewSpaceStartPos.xyz);
        float3 screenSpaceEndPosition = ProjectionPosition(vp.Projection, viewSpaceStartPos.xyz + viewSpaceReflectDirection);
        float3 screenSpaceDirection = normalize(screenSpaceEndPosition - screenSpaceStartPosition);

        //float4 startFrag = float4(fragPos, 1.f);
        //startFrag        = mul(viewProj, startFrag);
        ////startFrag.xy     = startFrag.xy * 0.5 + 0.5;
        //startFrag.xyz   /= startFrag.w;
        //startFrag.xy     = startFrag.xy * 0.5 + 0.5;
        //startFrag.y      = 1.f - startFrag.y;
        ////startFrag.xy     = startFrag.xy * texSize;
        //
        //float4 endFrag   = float4(targetPosition, 1.f);
        //endFrag          = mul(viewProj, endFrag);
        ////endFrag.xy       = endFrag.xy * 0.5 + 0.5;
        //endFrag.xyz     /= endFrag.w;
        //endFrag.xy       = endFrag.xy * 0.5 + 0.5;
        //endFrag.y        = 1.f - endFrag.y;
        ////endFrag.xy       = endFrag.xy * texSize;

        //float3 screenSpaceDirection = normalize(endFrag.xyz - startFrag.xyz);
        //float uv_z = LoadDepth(input.texCoord, 0);
        //float3 screen_xyz = float3(input.texCoord.x, input.texCoord.y, uv_z);

        //float3 reflectDir_ndc = normalize(mul(viewProj, float4(dir_world, 0.0)).xyz);

        bool valid_hit;
        uint max_traversal_intersections = ssrBuffer.g_max_traversal_intersections;
        float depth; 
        bool initialOnSurface = false;
        float3 finalPosition;
        float _t;
        float3 txyz, _boundary_planes;

        float3 color = SSR_Trace(screenSpaceStartPosition.xyz, screenSpaceDirection, 
            max_traversal_intersections, 
            valid_hit, 
            depth, 
            initialOnSurface,
            finalPosition,
            _t,
            txyz,
            _boundary_planes);

        //float3 p0 = startFrag.xyz;
        //float3 p1;
        //p1.xy = p0.xy + float2(0.02f, 0.02f);
        //p1.z = LoadDepth(p1.xy, 0);

        //output.color = float4(depth, depth, depth, 1.f);
        //output.color = float4(p1 - p0, 1.f);
        //if(initialOnSurface){
        //    output.color = float4(1.f, 0.f, 0.f, 1.f);
        //}
        //else{
        //    output.color = float4(0.f, 1.f, 0.f, 1.f);
        //}
        //output.color = float4(screenSpaceDirection, 1.f);
        //SSRRender[texCoord.xy] = float4(startFrag.xy, 0.f, 1.f);
        //float3 render = SampleRender(startFrag.xy);
        //float depth_ = LoadDepth(startFrag.xy, 0);
        //SSRRender[texCoord.xy] = float4(depth_, depth_, depth_, 1.f);
        
        int2 hiz_res = hiz.hizDimensions[6].xy;
        float depth0 = SampleDepth(screenSpaceStartPosition.xy * hiz_res, 6);
        output.color = float4(color, 1.f) * 1e-10 + float4(depth0, depth0, depth0, 1.f);
        //if (valid_hit){
        //    output.color = float4(color, 1.f) * 1e-10 + float4(1.f, 0.f, 0.f, 1.f);
        //    //SSRRender[texCoord.xy] = float4(1.f, 0.f, 0.f, 1.f);
        //}
        //else{
        //    //output.color = float4(0.f, 0.f, 0.f, 0.f);
        //    output.color = float4(0.f, 1.f, 0.f, 1.f);
        //    //SSRRender[texCoord.xy] = float4(0.f, 1.f, 0.f, 1.f);
        //}
    }
    else{
        output.color = float4(0.f, 0.f, 0.f, 0.f);
    }

    return output;
}
