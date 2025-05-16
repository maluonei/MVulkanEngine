#ifndef SHARE_SSR_H
#define SHARE_SSR_H

#ifdef SHARED_CODE_HLSL
#include "HLSLTypeAlise.h"
#else
#pragma once
#include "Shaders/share/HLSLTypeAlise.h"
#endif

struct SSRConfigBuffer{
    int doSSR;
};

#endif