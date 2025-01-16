struct UnifomBuffer0
{
    float4x4 viewProj;

    uint4 mipResolutions[13];
    
    float3 cameraPos;
    uint min_traversal_occupancy;

    int GbufferWidth;
    int GbufferHeight;
    int maxMipLevel;
    uint g_max_traversal_intersections;
};

[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UnifomBuffer0 ubo0;
}

[[vk::binding(1, 0)]]Texture2D<float4> gBufferNormal : register(t1);
[[vk::binding(2, 0)]]Texture2D<float4> gBufferPosition : register(t2);
[[vk::binding(3, 0)]]Texture2D<uint4>  gMatId : register(t3);
[[vk::binding(4, 0)]]Texture2D<float4> Render : register(t4);
[[vk::binding(5, 0)]]Texture2D<float>  Depth[13] : register(t5);

[[vk::binding(6, 0)]]SamplerState linearSampler : register(s0);
[[vk::binding(7, 0)]]SamplerState nearestSampler : register(s0);

#define FLOAT_MAX                          3.402823466e+38

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

void InitialAdvanceRay(float3 origin, float3 direction, float3 inv_direction, 
    int currentMipLevel,
    float2 floor_offset, float2 uv_offset, 
    out float3 position, out float current_t)
{
    float2 current_mip_resolution = ubo0.mipResolutions[currentMipLevel];
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

bool AdvanceRay(float3 origin, float3 direction, float3 inv_direction, 
    int currentMipLevel,float2 floor_offset, float2 uv_offset, 
    float surface_z, inout float3 position, inout float current_t) 
{
    float2 current_mip_resolution = ubo0.mipResolutions[currentMipLevel];
    float2 current_mip_resolution_inv = 1.0 / current_mip_resolution;

    float2 current_mip_position = current_mip_resolution * position.xy;
    
    float2 xy_plane = floor(current_mip_position) + floor_offset;
    xy_plane = xy_plane * current_mip_resolution_inv + uv_offset;
    float3 boundary_planes = float3(xy_plane, surface_z);

    // Intersect ray with the half box that is pointing away from the ray origin.
    // o + d * t = p' => t = (p' - o) / d
    float3 t = (boundary_planes - origin) * inv_direction;

    t.z = direction.z > 0 ? t.z : FLOAT_MAX;

    float t_min = min(min(t.x, t.y), t.z);

    bool above_surface = surface_z > position.z;

    bool skipped_tile = t_min != t.z && above_surface;

    // Make sure to only advance the ray if we're still above the surface.
    current_t = above_surface ? t_min : current_t;

    // Advance ray
    position = origin + current_t * direction;

    return skipped_tile;
}

float LoadDepth(float2 uv, int mipLevel){
    uint2 mipResolution = ubo0.mipResolutions[mipLevel];
    uv = uv / mipResolution;
    uv.y = 1.f - uv.y;
    return Depth[mipLevel].Sample(nearestSampler, uv);
}

float3 SSR_Trace(float3 origin, float3 direction,
    uint max_traversal_intersections, out bool valid_hit)
{
    float3 color = float3(0.f, 0.f, 0.f);
    const float3 inv_direction = select(direction != 0, 0.99999f / direction, FLOAT_MAX);

    float2 uv_offset = 0.005 * exp2(0) / float2(ubo0.GbufferWidth, ubo0.GbufferHeight);
    uv_offset = select(direction.xy < 0, -uv_offset, uv_offset);

    // Offset applied depending on current mip resolution to move the boundary to the left/right upper/lower border depending on ray direction.
    float2 floor_offset = select(direction.xy < 0, float2(0.f, 0.f), float2(1.f, 1.f));

    int currentMipLevel = 0;

    float current_t = 0.f;
    float3 position;
    InitialAdvanceRay(origin, direction, inv_direction, 
        currentMipLevel,
        floor_offset, uv_offset, position, current_t);

    int i = 0;
    bool exit_due_to_low_occupancy = false;

    while(i < max_traversal_intersections && currentMipLevel >=0){
        float2 current_mip_resolution = ubo0.mipResolutions[currentMipLevel];

        float2 current_mip_position = current_mip_resolution * position.xy;
    
        float surface_z = LoadDepth(current_mip_position, currentMipLevel);

        bool skipped_tile = AdvanceRay(origin, direction, inv_direction, 
            currentMipLevel, floor_offset, uv_offset, 
            surface_z, position, current_t);

        currentMipLevel += skipped_tile ? 1 : -1;
        ++i;
    }

    valid_hit = (i <= max_traversal_intersections) && (currentMipLevel <=0);
    if(position.x<=0 || position.x>=0.999 || position.y<=0 || position.y>=0.999)
        return float3(0.f, 0.f, 0.f);
    
    float2 uv = float2(position.x, 1.f-position.y);
    return Render.Sample(linearSampler, uv).rgb;
}

PSOutput main(PSInput input)
{
    PSOutput output;
    float2 texSize = float2(ubo0.GbufferWidth, ubo0.GbufferHeight);
    
    float4 gBufferValue0 = gBufferNormal.Sample(linearSampler, input.texCoord);
    float4 gBufferValue1 = gBufferPosition.Sample(linearSampler, input.texCoord);
    uint4 gBufferValue4 = gMatId.Load(int3(input.texCoord.xy * float2(ubo0.GbufferWidth, ubo0.GbufferHeight), 0));
    float4 rendered = Render.Sample(linearSampler, input.texCoord);

    float3 fragNormal = normalize(gBufferValue0.rgb);
    float3 fragPos = gBufferValue1.rgb;
    int matId = gBufferValue4.r;

    output.color = float4(rendered.rgb, 1.f);
    //output.color = float4(0.f, 0.f, 0.f, 1.f);
    if(matId == 1){
        float3 viewDir = normalize(ubo0.cameraPos - fragPos);
        float3 reflectDir = normalize(reflect(-viewDir, fragNormal));

        float3 targetPosition = fragPos + reflectDir;

        float4 startFrag = mul(ubo0.viewProj, float4(fragPos, 1.f));
        startFrag.xyz   /= startFrag.w;
        startFrag.xy     = startFrag.xy * 0.5 + 0.5;
        //startFrag.xy     = startFrag.xy * texSize;
        
        float4 endFrag   = float4(targetPosition, 1.f);
        endFrag          = mul(ubo0.viewProj, endFrag);
        endFrag.xyz     /= endFrag.w;
        endFrag.xy       = endFrag.xy * 0.5 + 0.5;
        //endFrag.xy       = endFrag.xy * texSize;

        float3 screenDirection = normalize(endFrag.xyz - startFrag.xyz);

        bool valid_hit;
        uint max_traversal_intersections = ubo0.g_max_traversal_intersections;
        float3 color = SSR_Trace(startFrag.xyz, screenDirection, 
            max_traversal_intersections, valid_hit);

        if (valid_hit)
            output.color += 0.8f * float4(color, 1.f);
        //else
        //    output.color += float4(0.f, 0.f, 0.f, 1.f);
    }
    //else{
    //    output.color = float4(rendered.rgb, 1.f);
    //}

    return output;
}