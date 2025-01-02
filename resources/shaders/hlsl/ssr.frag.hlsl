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

[[vk::binding(1, 0)]]Texture2D<float4> gBufferPosition : register(t1);
[[vk::binding(2, 0)]]Texture2D<float4> gBufferNormal : register(t2);
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

#define maxDistance 15.
//float resolution  = 0.3;
#define maxSteps    10
//float thickness   = 0.5;

float3 SSR(float3 screenPosition, float3 screenDirection){
    float3 color = float3(0.f, 0.f, 0.f);
    float stepLength = 1e-4;

    for(int i=0; i<maxSteps; i++){
        float3 pos = screenPosition + screenDirection * i * stepLength;

        if(pos.x<0 || pos.x>1 || pos.y<0 || pos.y>1) break;

        float viewDepth = Depth.Sample(linearSampler, pos.xy).r;
        if(pos.z > viewDepth){
            color = Render.Sample(linearSampler, pos.xy).rgb;
            break;
        }
    }
    return color;
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

    if(matId == 1){
        float3 viewDir = normalize(ubo0.cameraPos - fragPos);
        float3 reflectDir = reflect(-viewDir, fragNormal);

        float3 targetPosition = fragPos + reflectDir * maxDistance;

        float4 startFrag = mul(ubo0.viewProj, float4(fragPos, 1.f));
        startFrag.xyz   /= startFrag.w;
        startFrag.xy     = startFrag.xy * 0.5 + 0.5;
        //startFrag.xy    *= texSize;  

        float4 endFrag   = float4(targetPosition, 1.f);
        endFrag          = mul(ubo0.viewProj, endFrag);
        endFrag.xyz     /= endFrag.w;
        endFrag.xy       = endFrag.xy * 0.5 + 0.5;
        //endFrag.xy      *= texSize;

        float3 screenDirection = normalize(endFrag.xyz - startFrag.xyz);

        float3 color = SSR(startFrag.xyz, screenDirection);
        output.color = float4(color, 1.f);
    }
    else{
        output.color = float4(0, 0, 0, 1);
        output.color = float4(rendered.rgb, 1.f);
    }

    //output.color += rendered;
    return output;
}