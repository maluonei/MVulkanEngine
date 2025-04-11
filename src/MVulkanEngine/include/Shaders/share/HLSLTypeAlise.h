// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/*================================================================================================
	HLSLTypeAliases.h: include this file in headers that are used in both C++ and HLSL
	to define type aliases that map HLSL types to C++ UE types.
=================================================================================================*/

#ifndef SHARED_CODE_HLSL
#include<glm/glm.hpp>

using int2 = glm::ivec2;
using int3 = glm::ivec3;
using int4 = glm::ivec4;

using uint = uint32_t;
using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

//dword
//half
using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using float3x3 = glm::mat3;
using float4x4 = glm::mat4;
#endif

//using FLWCVector3 = TLargeWorldRenderPosition<float>;


