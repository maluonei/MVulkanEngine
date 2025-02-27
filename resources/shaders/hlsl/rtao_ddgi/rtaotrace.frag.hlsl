struct Light
{
    float3 direction;
    float intensity;

    float3 color;
    int shadowMapIndex;
};

struct UniformBuffer0
{
    Light lights[2];

    float3 cameraPos;
    int lightNum;

    int resetAccumulatedBuffer;
    int gbufferWidth;
    int gbufferHeight;
    float padding2;
};

[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UniformBuffer0 ubo0;
}

[[vk::binding(1, 0)]]Texture2D<float4> BlueNoise : register(t0);
[[vk::binding(2, 0)]]SamplerState linearSampler : register(s0);

[[vk::binding(3, 0)]]RaytracingAccelerationStructure Tlas : register(t4);


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

float3 rgb2srgb(float3 color)
{
    color.x = color.x < 0.0031308 ? color.x * 12.92f : pow(color.x, 1.0 / 2.4) * 1.055f - 0.055f;
    color.y = color.y < 0.0031308 ? color.y * 12.92f : pow(color.y, 1.0 / 2.4) * 1.055f - 0.055f;
    color.z = color.z < 0.0031308 ? color.z * 12.92f : pow(color.z, 1.0 / 2.4) * 1.055f - 0.055f;

    return color;
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
        if(dot(randomDir, normal)<0) randomDir = -randomDir;
        float3 rayDirection = normalize(normal + randomDir); 

        ray.Origin = position + 0.00001f * normal;
        ray.Direction = rayDirection;

        if(RayTracingAnyHit(ray, t))
            //ao-=1.f;
            ao += pow(saturate(t/radius), 2.);
    }

    return 1 - (ao / rayCount);
}

float rtao2(float3 position, float3 normal, float2 uv, float radius, int noiseValue, int rayCount){
    float ao = 1.f;
    float t = 0.f;

    RayDesc ray;
    ray.TMin = 0.f;
    ray.TMax = radius;

    float3 blueNoiseUnitVector = SphericalFibonacci(clamp(noiseValue * c_numAngles, 0, rayCount-1), rayCount);
    float3 rayDirection = normalize(normal + blueNoiseUnitVector);
    
    ray.Origin = position + (normal * 0.0001);
    ray.Direction = normalize(normal + blueNoiseUnitVector);

    if(RayTracingAnyHit(ray, t))
        ao -= pow(saturate(t/radius), 2.);

    return ao;
}

PSOutput main(PSInput input)
{
    PSOutput output;

    float noiseValue = BlueNoise.Sample(linearSampler, input.texCoord).x;

    float3 fragcolor = float3(0.f, 0.f, 0.f);

    float3 V = normalize(ubo0.cameraPos.xyz - fragPos);
    float3 R = reflect(-V, fragNormal);

    float radius = 0.1;
    int rayCount = 8;

    float ao = rtao2(fragPos, fragNormal, input.texCoord, radius, noiseValue, rayCount);
    
    float3 aoColor = float3(ao, ao, ao);

    output.color = float4(aoColor, 1.f);
    
    return output;
}
