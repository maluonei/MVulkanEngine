struct UnifomBuffer0
{
    float4x4 viewProj;
    
    float3 cameraPos;
    float padding0;

    int GbufferWidth;
    int GbufferHeight;
    int padding1;
    int padding2;
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
[[vk::binding(5, 0)]]Texture2D<float4> Depth : register(t5);

[[vk::binding(6, 0)]]SamplerState linearSampler : register(s0);

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

float3 SSR_MiniStep(float3 screenPosition, float3 screenDirection, float stepLength, int maxSteps){
    float3 color = float3(0.f, 0.f, 0.f);

    for(int i=1; i<=maxSteps; i++){
        float3 pos = screenPosition + screenDirection * i * stepLength;

        if(pos.x<=0.001 || pos.x>=0.999 || pos.y<=0.001 || pos.y>=0.999){
            return float3(0.f, 0.f, 0.f);
        }

        float2 colorTex = float2(pos.x, 1.f - pos.y);
        
        int2 colorTexInt = int2(colorTex * float2(ubo0.GbufferWidth, ubo0.GbufferHeight));
        float viewDepth = Depth.Load(int3(colorTexInt, 0), 0).r;

        //float viewDepth = Depth.Sample(linearSampler, colorTex).r;
        if(pos.z > viewDepth){
            if(pos.z - viewDepth < 0.00001){
                color = Render.Sample(linearSampler, colorTex).rgb;
            }
            else{
                color = float3(0.f, 0.f, 0.f);
            }
            return color;
        }
    }

    return float3(0.f, 0.f, 0.f);
}

float3 SSR_SmallStep(float3 screenPosition, float3 screenDirection, float stepLength, int maxSteps){
    float3 color = float3(0.f, 0.f, 0.f);

    for(int i=1; i<=maxSteps; i++){
        float3 pos = screenPosition + screenDirection * i * stepLength;

        if(pos.x<=0. || pos.x>=1. || pos.y<=0. || pos.y>=1.){
            color = SSR_MiniStep(pos - screenDirection * stepLength, screenDirection, stepLength/10.f, 10);
            return color;
        }

        float2 colorTex = float2(pos.x, 1.f - pos.y);
        int2 colorTexInt = int2(colorTex * float2(ubo0.GbufferWidth, ubo0.GbufferHeight));
        float viewDepth = Depth.Load(int3(colorTexInt, 0), 0).r;

        //float viewDepth = Depth.Sample(linearSampler, colorTex).r;
        if(pos.z > viewDepth){
            color = SSR_MiniStep(pos - screenDirection * stepLength, screenDirection, stepLength/10.f, 10);
            return color;
        }
    }
    return float3(0.f, 0.f, 0.f);
}

float3 SSR_LargeStep(float3 screenPosition, float3 screenDirection, float stepLength, int maxSteps){
    float3 color = float3(0.f, 0.f, 0.f);

    for(int i=1; i<maxSteps; i++){
        float3 pos = screenPosition + screenDirection * i * stepLength;

        if(pos.x<=0. || pos.x>=1. || pos.y<=0. || pos.y>=1.){
            color = SSR_SmallStep(pos - screenDirection * stepLength, screenDirection, stepLength/10.f, 10);
            return color;
        }

        float2 colorTex = float2(pos.x, 1.f - pos.y);
        int2 colorTexInt = int2(colorTex * float2(ubo0.GbufferWidth, ubo0.GbufferHeight));
        float viewDepth = Depth.Load(int3(colorTexInt, 0), 0).r;

        //float viewDepth = Depth.Sample(linearSampler, colorTex).r;
        if(pos.z > viewDepth){
            color = SSR_SmallStep(pos - screenDirection * stepLength, screenDirection, stepLength/10.f, 10);
            return color;
        }
    }
    return float3(0.f, 1.f, 0.f);
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
    if(matId == 1){
        float3 viewDir = normalize(ubo0.cameraPos - fragPos);
        float3 reflectDir = normalize(reflect(-viewDir, fragNormal));

        float3 targetPosition = fragPos + reflectDir;

        float4 startFrag = mul(ubo0.viewProj, float4(fragPos, 1.f));
        startFrag.xyz   /= startFrag.w;
        startFrag.xy     = startFrag.xy * 0.5 + 0.5;

        float4 endFrag   = float4(targetPosition, 1.f);
        endFrag          = mul(ubo0.viewProj, endFrag);
        endFrag.xyz     /= endFrag.w;
        endFrag.xy       = endFrag.xy * 0.5 + 0.5;

        float3 screenDirection = normalize(endFrag.xyz - startFrag.xyz);

        float3 color = SSR_LargeStep(startFrag.xyz, screenDirection, 5e-2, 40);

        output.color += float4(color, 1.f);
    }

    return output;
}