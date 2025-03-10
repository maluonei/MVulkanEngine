#pragma once
#ifndef SHADER_H
#define SHADER_H

#include "string"
//#include "SPIRV/GlslangToSpv.h"

#include "Utils/VulkanUtil.hpp"
#include <vector>
//#include <shaderc/shaderc.hpp>


class Shader {
public:
	//Shader(std::string vertPath, std::string fragPath);
	Shader(std::string entryPoint="main");
	Shader(std::string path, ShaderStageFlagBits _stage, std::string entryPoint = "main", bool compileEverytime = false);

	void Compile();
	inline ShaderStageFlagBits GetShaderStage()const { return m_shaderStage; }
	inline std::vector<char> GetCompiledShaderCode() const { return m_compiledShaderCode; }
	inline std::string GetCompiledShaderPath() const { return m_compiledShaderPath; };
private:
	void compile(std::string shaderPath);
	ShaderStageFlagBits m_shaderStage;
	bool				m_compileEverytime = false;	
	std::string			m_shaderPath;
	std::string			m_compiledShaderPath;
	std::vector<char>	m_compiledShaderCode;
	std::string			m_entryPoint;
};


#endif