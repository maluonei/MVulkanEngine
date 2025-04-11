//#ifdef SHADER_CODE_CPP
//#include "Shaders/share/HLSLTypeAlise.h"
//#elif defined(SHARED_CODE_HLSL)
//#include "HLSLTypeAlise.h"
//#endif
#ifdef SHARED_CODE_HLSL
#include "HLSLTypeAlise.h"
#else
#include "Shaders/share/HLSLTypeAlise.h"
#endif

#define MAX_LIGHTS 4

struct MLight{
    float4x4 shadowViewProj;

    float3 direction;
    float intensity;

    float3 color;
    int shadowMapIndex;

    int2 shadowmapResolution;
    float shadowCameraZnear;
    float shadowCameraZfar;
    //float padding6;
    //float padding7;
};

//struct MLightBuffer{
//    MLight lights[MAX_LIGHTS];
//};

struct LightBuffer{
    MLight lights[MAX_LIGHTS];

    //float3 cameraPos;
    int lightNum;
    int padding0;
    int padding1;
    int padding2;
    
    //int ResolutionWidth;
    //int ResolutionHeight;
    //int WindowResWidth;
    //int WindowResHeight;
};

struct MCameraBuffer {
    float3 cameraPos;
    float padding0;
    float3 cameraDir;
    float padding1;
};

struct MScreenBuffer {
    int2 WindowRes;
};

