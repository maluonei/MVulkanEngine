#include "Common.h"

struct PSInput
{
    float4 position : SV_POSITION;
};

struct PSOutput
{
    [[vk::location(0)]] uint4 color : SV_TARGET0; // 16 bytes normalx, 16bytes normaly, 16bytes normalz, 16bytes positionx, 16bytes positiony, 16bytes positionz, 16bytes u, 16bytes v
};

PSOutput main(PSInput input)
{
    PSOutput output;
    output.color = uint4(0, 0, 0, 1);

    return output;
}