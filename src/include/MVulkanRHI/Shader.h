#pragma once
#ifndef SHADER_H
#define SHADER_H

#include "string"
//#include "SPIRV/GlslangToSpv.h"

#include "Utils/VulkanUtil.h"
#include <vector>
//#include <shaderc/shaderc.hpp>


class Shader {
public:
	//Shader(std::string vertPath, std::string fragPath);
	Shader();
	Shader(std::string path, ShaderStageFlagBits _stage);

	void Compile();
	inline ShaderStageFlagBits GetShaderStage()const { return shaderStage; }
	inline std::vector<char> GetCompiledShaderCode() const { return compiledShaderCode; }
	inline std::string GetCompiledShaderPath() const { return compiledShaderPath; };
private:
	void compile(std::string shaderPath);
	ShaderStageFlagBits shaderStage;
	std::string shaderPath;
	std::string compiledShaderPath;
	std::vector<char> compiledShaderCode;
};


#endif