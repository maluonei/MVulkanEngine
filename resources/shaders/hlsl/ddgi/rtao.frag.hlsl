struct UniformBuffer0
{
    int resetAccumulatedBuffer;
    int gbufferWidth;
    int gbufferHeight;
    int padding0;
};

[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UniformBuffer0 ubo0;
}

[[vk::binding(1, 0)]]Texture2D<float4> gBufferNormal : register(t0);
[[vk::binding(2, 0)]]Texture2D<float4> gBufferPosition : register(t1);
[[vk::binding(3, 0)]]RWTexture2D<float2> accumulatedBuffer : register(u0);

[[vk::binding(4, 0)]]SamplerState linearSampler : register(s0);

[[vk::binding(5, 0)]]RaytracingAccelerationStructure Tlas : register(t4);


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

static const float PI = 3.14159265359f;

bool RayTracingAnyHit(in RayDesc rayDesc, out float t) {
  uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

  RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;

  q.TraceRayInline(Tlas, rayFlags, 0xFF, rayDesc);
  q.Proceed();

  if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
    t = q.CommittedRayT();
    return true;
  }

  return false;
}

float RandomWithOffset(float2 uv, int offset)
{
    float n = dot(uv + float2(offset, offset), float2(12.9898, 78.233));
    n = frac(sin(n) * 43758.5453123);
    return n;
}

float3 RandomDirection(float2 uv, int u1, int u2){
    float theta = RandomWithOffset(uv, u1) * 2 * PI;
    float phi = RandomWithOffset(uv, u2) * 2 * PI;

    float3 randomDir = float3(
        cos(phi) * cos(theta),
        sin(phi),
        cos(phi) * sin(theta)
    );

    return randomDir;
}

float3 SphericalFibonacci(float index, float numSamples)
{
    const float b = (sqrt(5.f) * 0.5f + 0.5f) - 1.f;
    float phi = 2 * PI * frac(index * b);
    float cosTheta = 1.f - (2.f * index + 1.f) * (1.f / numSamples);
    float sinTheta = sqrt(saturate(1.f - (cosTheta * cosTheta)));

    return float3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta);
}

float rtao(float3 position, float3 normal, float2 uv, int w, float radius, int rayCount){
    //float ao = rayCount;
    float ao = 0.f;
    float t = 0.f;

    RayDesc ray;
    //ray.Origin = position + 0.00001f * normal;
    ray.TMin = 0.f;
    ray.TMax = radius;

    for(int i=0;i<rayCount;i++){
        float3 randomDir = RandomDirection(uv, 2*i + w, 2*i+1 + w);
        //if(dot(randomDir, normal)<0) randomDir = -randomDir;
        float3 rayDirection = normalize(normal + randomDir); 

        ray.Origin = position + 0.001f * normal;
        ray.Direction = rayDirection;

        if(RayTracingAnyHit(ray, t))
            //ao-=1.f;
            //ao += pow(saturate(t/radius), 2.);
            ao += 1.f;
    }

    return 1 - (ao / rayCount);
}

float rtao2(float3 position, float3 normal, float2 uv, int w, float radius, int rayCount){
    //float ao = rayCount;
    float ao = 0.f;
    float t = 0.f;

    RayDesc ray;
    //ray.Origin = position + 0.00001f * normal;
    ray.TMin = 0.f;
    ray.TMax = radius;

    for(int i=0;i<rayCount;i++){
        float3 blueNoiseUnitVector = SphericalFibonacci(i, rayCount);
        float3 rayDirection = normalize(normal + blueNoiseUnitVector);
        
        ray.Origin = position + (normal * 0.0001);
        ray.Direction = normalize(normal + blueNoiseUnitVector);

        if(RayTracingAnyHit(ray, t))
            //ao-=1.f;
            ao += pow(saturate(t/radius), 2.);
    }

    return 1 - (ao / rayCount);
}

PSOutput main(PSInput input)
{
    PSOutput output;
    
    float4 gBufferValue0 = gBufferNormal.Sample(linearSampler, input.texCoord);
    float4 gBufferValue1 = gBufferPosition.Sample(linearSampler, input.texCoord);
    float2 accumulatedBufferValue = accumulatedBuffer.Load(int3(input.texCoord * int2(ubo0.gbufferWidth, ubo0.gbufferHeight), 0));

    float3 fragNormal = normalize(gBufferValue0.rgb);
    float3 fragPos = gBufferValue1.rgb;

    float accumulatedAO = ubo0.resetAccumulatedBuffer==0? accumulatedBufferValue.x:0.f;
    float accumulatedFrameCount = ubo0.resetAccumulatedBuffer==0? accumulatedBufferValue.y:0.f;
    
    float3 fragcolor = float3(0.f, 0.f, 0.f);

    float radius = 0.5f;
    int rayCount = 4;

    float ao = rtao(fragPos, fragNormal, input.texCoord, int(rayCount * accumulatedFrameCount), radius, rayCount);
    float finalAO = (accumulatedAO * accumulatedFrameCount + ao) / (accumulatedFrameCount+1);
    
    accumulatedBuffer[input.texCoord * int2(ubo0.gbufferWidth, ubo0.gbufferHeight)] = float2(finalAO, accumulatedFrameCount+1);
    
    //output.color = float4(ao, ao, ao, 1.f);
    float3 aoColor = float3(finalAO, finalAO, finalAO); 

    //output.color = float4(finalAO * fragcolor, 1.f);
    output.color = float4(aoColor + fragcolor * 0.00000000000001, 1.f);
    
    return output;
} 
