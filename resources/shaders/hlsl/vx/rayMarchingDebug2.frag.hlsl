struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
    [[vk::location(1)]] float4 Color_Debug0 : SV_TARGET1;
    [[vk::location(2)]] float4 Color_Debug1 : SV_TARGET2;
    [[vk::location(3)]] float4 Color_Debug2 : SV_TARGET3;
    [[vk::location(4)]] float4 Color_Debug3 : SV_TARGET4;
    //[[vk::location(4)]] float4 Color_Debug3 : SV_TARGET4;
    //[[vk::location(5)]] float4 Color_Debug4 : SV_TARGET5;
    //[[vk::location(6)]] float4 Color_Debug5 : SV_TARGET6;
    //[[vk::location(7)]] float4 Color_Debug6 : SV_TARGET7;
};

struct UniformBuffer0
{
    float3 aabbMin;
    float padding;
    float3 aabbMax;
    float padding2;

    uint3 gridResolution;
	float padding3;
};

struct UniformBuffer1{
    float3 cameraPos;
    float padding;

    float3 cameraDir;
    uint maxRaymarchSteps;

    int2 ScreenResolution;
    float fovY;
    float zNear;
};

struct UniformBuffer2{
    float3 lightDir;
    float padding2;

    float3 lightColor;
    float lightIntensity;
};

struct RayMarchSDFStruct{
    float depth;
    int step;
    float sdfValue;
    float3 debuginfo0;
    float3 debuginfo1;
    float3 debuginfo2;
    float3 debuginfo3;
    float3 debuginfo4;
    float3 debuginfo5;
    float3 debuginfo6;
    float3 debuginfo7;
    float3 debuginfo8;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    UniformBuffer0 ubo0;
};

[[vk::binding(1, 0)]]
cbuffer ubo1 : register(b1)
{
    UniformBuffer1 ubo1;
};

[[vk::binding(2, 0)]]
cbuffer ubo2 : register(b2)
{
    UniformBuffer2 ubo2;
};

[[vk::binding(3, 0)]] RWTexture3D<float> SDFTexture : register(u2);


//#define epsilon 1e-3

void Swap(inout float a, inout float b) {
    float temp = a;
    a = b;
    b = temp;
}

float3 GetPixelDirection(float2 uv){
    float3 UP = float3(0.f, 1.f, 0.f);
    float aspectRatio = (float)ubo1.ScreenResolution.x / (float)ubo1.ScreenResolution.y;

    float3 forward = normalize(ubo1.cameraDir);
    float3 right = cross(forward, UP);
    float3 up = cross(right, forward);

    float halfHeight = tan(ubo1.fovY * 0.5f) * ubo1.zNear;
    float halfWidth = aspectRatio * halfHeight;

    float3 zPlaneCenter = ubo1.zNear * forward + ubo1.cameraPos;
    float3 zPlaneCorner = zPlaneCenter - right * halfWidth + up * halfHeight;

    //float2 pixelOffset = 0.5f / float2(ubo1.ScreenResolution);
    float3 pixelPos = zPlaneCorner + right * uv.x * 2 * halfWidth - up * uv.y * 2 * halfHeight;

    return normalize(pixelPos - ubo1.cameraPos);
}

float3 GetSDFGridPosition(uint3 gridCoord){
    float3 offset = (ubo0.aabbMax - ubo0.aabbMin) / float3(ubo0.gridResolution - 1);
    return ubo0.aabbMin + gridCoord * offset;
}

float GetSDFValue(float3 position, out bool inSDFTex, out RayMarchSDFStruct debug){
    float3 offset = 0.001f * (ubo0.aabbMax - ubo0.aabbMin) / float3(ubo0.gridResolution);
    float epsilon = offset;
    //debug = 0;

    if( position.x < (ubo0.aabbMin.x - epsilon) || position.x > (ubo0.aabbMax.x + epsilon) || 
        position.y < (ubo0.aabbMin.y - epsilon) || position.y > (ubo0.aabbMax.y + epsilon) || 
        position.z < (ubo0.aabbMin.z - epsilon) || position.z > (ubo0.aabbMax.z + epsilon))
    {
        inSDFTex = false;

        //float3 offset = 0.5f / float3(ubo0.gridResolution);
        uint3 gridCoord = uint3(((position - ubo0.aabbMin) / (ubo0.aabbMax - ubo0.aabbMin)) * (ubo0.gridResolution));
        debug.debuginfo7.xyz = 0.5f * gridCoord / float3(ubo0.gridResolution-1);

        return -1.f;
    }

    inSDFTex = true;

    uint3 gridCoord = uint3(((position - ubo0.aabbMin) / (ubo0.aabbMax - ubo0.aabbMin)) * float3(ubo0.gridResolution));
    gridCoord.x = clamp(gridCoord.x, 0, ubo0.gridResolution.x-1);
    gridCoord.y = clamp(gridCoord.y, 0, ubo0.gridResolution.y-1);
    gridCoord.z = clamp(gridCoord.z, 0, ubo0.gridResolution.z-1);
    
    //return 10.f;
    return SDFTexture.Load(gridCoord);
}

float2 GetNearestDistance(
    float3 position, float3 direction, 
    float3 aabbMin, float3 aabbMax,
    out bool hit){
    float tMin = -1e20f;
    float tMax = 1e20f;

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
        hit = false;
        return float2(-1.f, -1.f);
    }

    hit = true;
    if(tMin < 0 && tMax > 0) float2(0.f, tMax);

    return float2(tMin, tMax);
} 

float GetNextPixelSDF(float3 position, float3 direction){
    int3 currentIndex = int3(float3((position - ubo0.aabbMin) / (ubo0.aabbMax - ubo0.aabbMin)) * (ubo0.gridResolution));
    //currentIndex = clamp(currentIndex, 0, ubo0.gridResolution-1);
    float3 voxelSize = (ubo0.aabbMax - ubo0.aabbMin) / (ubo0.gridResolution);

    float3 currentAabbMin = ubo0.aabbMin + currentIndex * voxelSize;
    float3 currentAabbMax = currentAabbMin + voxelSize;

    bool hit = false;
    float2 nextHitT = GetNearestDistance(position, direction, currentAabbMin, currentAabbMax, hit);
    if(hit) 
        return nextHitT.y + 2e-3 * voxelSize.x;
    else
        return 0.5f * voxelSize.x;
}

int3 RayMarchSDF(float3 position, float3 direction, out RayMarchSDFStruct struc){
    float start = 0.f;
    bool hit = false;
    float2 aabbT = GetNearestDistance(position, direction, ubo0.aabbMin, ubo0.aabbMax, hit);
    float voxelSize = (ubo0.aabbMax - ubo0.aabbMin) / (ubo0.gridResolution);
    struc.depth = aabbT.x + 1e-3 * voxelSize.x;
    struc.depth = max(0.f, struc.depth);

    if(!hit) return uint3(-1, -1, -1);

    struc.step=0;

    for(;struc.step<ubo1.maxRaymarchSteps && struc.depth<=aabbT.y;struc.step++){
        float3 pos = position + direction * struc.depth;// * gridOffset;
        struc.debuginfo0.xyz = pos + 0.2f;

        bool inSdf = false;
        int debug = 0;
        float sdfValue = GetSDFValue(pos, inSdf, struc);

        struc.sdfValue = sdfValue;

        if(!inSdf){
            struc.depth = -1.f;
            //struc.debuginfo8 = struc.debuginfo7;
            return uint3(-3, -3, -3);
        }

        if(sdfValue == 0.f){
            return uint3(1, 1, 1);
        }

        if(sdfValue > 0.f && sdfValue < 2.f){
            float nextPosSdf = GetNextPixelSDF(pos, direction);
            struc.depth += nextPosSdf;
        }
        else{
            struc.depth += (sdfValue - 1.f) * voxelSize.x;
        }
        //struc.depth = max(0.f, struc.depth);
    }

    return int3(-2, -2, -2);
} 

PSOutput main(PSInput input)
{
    PSOutput output;
    //float depth = 0.f;
    //int step = 0;
    RayMarchSDFStruct struc;

    struc.step = 0;
    float3 camDir = GetPixelDirection(input.texCoord);
    float nearestDis = GetNearestDistance(ubo1.cameraPos, camDir, ubo0.aabbMin, ubo0.aabbMax, struc.depth);
    //depth = 0.f;
    
    uint3 res = RayMarchSDF(ubo1.cameraPos, camDir, struc);
    
    output.Color = float4(0.f, 0.f, 0.f, 1.f);
    output.Color_Debug0 = float4(0.f, 0.f, 0.f, 1.f);
    output.Color_Debug1 = float4(0.f, 0.f, 0.f, 1.f);
    output.Color_Debug2 = float4(0.f, 0.f, 0.f, 1.f);
    output.Color_Debug3 = float4(0.f, 0.f, 0.f, 1.f);

    if(res.x == -1 && res.y == -1 && res.z == -1){
        output.Color = float4(0.f, 0.f, 0.f, 1.f);
        output.Color_Debug0 = float4(1.f, 1.f, 1.f, 1.f);
        output.Color_Debug1 = float4(1.f, 1.f, 1.f, 1.f);
        output.Color_Debug3 = float4(0.f, 0.f, 1.f, 1.f);
        return output;
    }

    if( (res.x == 1 && res.y == 1 && res.z == 1)){
        float3 hitPoint = ubo1.cameraPos + camDir * struc.depth;
        float3 posCoord = (hitPoint - ubo0.aabbMin) / (ubo0.aabbMax - ubo0.aabbMin);
        output.Color_Debug0 = float4(posCoord, 1.f);
        output.Color_Debug1 = float4(struc.sdfValue / 10.f * float3(1.f, 1.f, 1.f), 1.f);
        output.Color_Debug2 = float4(struc.step / 100.f * float3(1.f, 1.f, 1.f), 1.f);
        output.Color = float4(posCoord, 1.f);
        output.Color_Debug3 = float4(1.f, 0.f, 0.f, 1.f);
    }

    if( (res.x == -2 && res.y == -2 && res.z == -2)){
        float3 hitPoint = ubo1.cameraPos + camDir * struc.depth;
        float3 posCoord = (hitPoint - ubo0.aabbMin) / (ubo0.aabbMax - ubo0.aabbMin);
        output.Color_Debug0 = float4(posCoord, 1.f);
        output.Color_Debug1 = float4(struc.sdfValue / 10.f * float3(1.f, 1.f, 1.f), 1.f);
        output.Color_Debug2 = float4(struc.step / 100.f * float3(1.f, 1.f, 1.f), 1.f);
        output.Color_Debug3 = float4(0.f, 1.f, 0.f, 1.f);
    }

    if( (res.x == -3 && res.y == -3 && res.z == -3)){
        float3 hitPoint = ubo1.cameraPos + camDir * struc.depth;
        float3 posCoord = (hitPoint - ubo0.aabbMin) / (ubo0.aabbMax - ubo0.aabbMin);
        output.Color_Debug0 = float4(posCoord, 1.f);
        output.Color_Debug1 = float4(0.f, 1.f, 0.f, 1.f);
        output.Color_Debug2 = float4(struc.step / 100.f * float3(1.f, 1.f, 1.f), 1.f);
        output.Color_Debug3 = float4(1.f, 1.f, 0.f, 1.f);
    }

    return output;
}