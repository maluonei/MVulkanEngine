#include "Common.h"
#include "SSR.h"
#include "shading.hlsli"
#include "util.hlsli"

//[[vk::binding(0, 0)]]
//cbuffer lightBuffer : register(b0)
//{
//    LightBuffer lightBuffer;
//}
//
//[[vk::binding(1, 0)]]
//cbuffer cameraBuffer : register(b1)
//{
//    MCameraBuffer cameraBuffer;
//}

[[vk::binding(0, 0)]]
cbuffer screenBuffer : register(b0)
{
    MScreenBuffer screenBuffer;
}

[[vk::binding(1, 0)]]
cbuffer ssrConfigBuffer : register(b1)
{
    SSRConfigBuffer ssrConfig;
}

//[[vk::binding(7, 0)]]
//cbuffer intBuffer : register(b3)
//{
//    OutputContent outputContent;
//}

[[vk::binding(2, 0)]]Texture2D<float4> Render : register(t0);
[[vk::binding(3, 0)]]Texture2D<float4> SSR : register(t1);
//[[vk::binding(6, 0)]]SamplerState linearSampler : register(s0);

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

PSOutput main(PSInput input)
{
    PSOutput output;

    bool doSSR = (ssrConfig.doSSR == 1);

    uint3 coord = uint3(uint2(input.texCoord * float2(screenBuffer.WindowRes.x, screenBuffer.WindowRes.y)), 0);
    
    float4 render = Render.Load(coord);
    float4 ssr = SSR.Load(coord);

    if(doSSR){
        output.color = float4(render.rgb + ssr.rgb, 1.f);
    }
    else{
        output.color = render;
    }

    return output;
}
