#include "Shaders/Shader.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

Shader::Shader()
{
}


Shader::Shader(std::string _path, ShaderStageFlagBits _stage, bool compileEveryTime)
{
	m_shaderPath = _path;
	m_shaderStage = _stage;
	m_compileEverytime = compileEveryTime;
}

void Shader::Compile() {
	compile(m_shaderPath);
}

void Shader::compile(std::string shaderPath)
{
	fs::path projectRootPath = PROJECT_ROOT;
	fs::path shaderRootPath = projectRootPath / "resources" / "shaders";

	//fs::path compilerPath = projectRootPath / "build" / "ShaderCompilers" / "glslangValidator.exe";

	spdlog::info("compilng shader: {0}", shaderPath);

	bool needToCompile = false;

	if (shaderPath.ends_with("glsl")) {
		fs::path compilerPath = projectRootPath / "build" / "ShaderCompilers" / "glslangValidator.exe";

		size_t size = shaderPath.size();

		fs::path glslShader = shaderRootPath / (shaderPath);
		fs::path outputShader = shaderRootPath / (shaderPath.substr(0, size - 5) + ".spv");

		if (!fs::exists(glslShader)) {
			spdlog::error("source shader don't exist: {0}", shaderPath);
		}

		if (!fs::exists(outputShader)) {
			needToCompile = true;
		}
		else {
			auto timeSourceFile = fs::last_write_time(glslShader);
			auto timeOutputFile = fs::last_write_time(outputShader);

			if (timeSourceFile > timeOutputFile) {
				needToCompile = true;
			}
		}

		if (m_compileEverytime || needToCompile) {
			fs::path logFile = shaderRootPath / "log.txt";
			std::string command = compilerPath.string() + " -V --target-env vulkan1.3 " + glslShader.string() + " -o " + outputShader.string() + " > " + logFile.string();
			int status = system(command.c_str());

			if (status != 0) {
				std::ifstream file(logFile.string());
				std::string line;

				spdlog::error("fail to compile shader:");
				while (std::getline(file, line)) {
					spdlog::error(line);
				}
			}
		}
		m_compiledShaderCode = readFile(outputShader.string());
		m_compiledShaderPath = outputShader.string();
	}
	else if (shaderPath.ends_with("hlsl")) {
		fs::path compilerPath = projectRootPath / "build" / "ShaderCompilers" / "dxc.exe";
		//fs::path compilerPath = projectRootPath / "build" / "ShaderCompilers" / "glslangValidator.exe";

		size_t size = shaderPath.size();

		fs::path hlslShader = shaderRootPath / (shaderPath);
		fs::path outputShader = shaderRootPath / (shaderPath.substr(0, size - 5) + ".spv");

		if (!fs::exists(hlslShader)) {
			spdlog::error("source shader don't exist: {0}", shaderPath);
		}

		if (!fs::exists(outputShader)) {
			needToCompile = true;
		}
		else {
			auto timeSourceFile = fs::last_write_time(hlslShader);
			auto timeOutputFile = fs::last_write_time(outputShader);

			if (timeSourceFile > timeOutputFile) {
				needToCompile = true;
			}
		}

		if (m_compileEverytime || needToCompile) {
			std::string phase = "";
			if (shaderPath.substr(size - 9, 4) == "vert") {
				phase = "vs_6_4";
			}
			else if (shaderPath.substr(size - 9, 4) == "frag") {
				phase = "ps_6_4";
			}
			else if (shaderPath.substr(size - 9, 4) == "geom") {
				phase = "gs_6_4";
			}
			else if (shaderPath.substr(size - 9, 4) == "comp") {
				phase = "cs_6_4";
			}

			//if (shaderPath.substr(size - 9, 4) == "vert") {
			//	phase = "vert";
			//}
			//else if (shaderPath.substr(size - 9, 4) == "frag") {
			//	phase = "frag";
			//}
			//else if (shaderPath.substr(size - 9, 4) == "geom") {
			//	phase = "geom";
			//}
			//else if (shaderPath.substr(size - 9, 4) == "comp") {
			//	phase = "comp";
			//}

			fs::path logFile = shaderRootPath / "log.txt";
			std::string command = compilerPath.string() + " -T" + phase + " -E main -spirv -fspv-preserve-bindings -fspv-target-env=vulkan1.3 -Fo " + outputShader.string() + " " + hlslShader.string() + " > " + logFile.string();
			//std::string command = compilerPath.string() + " -V --target-env vulkan1.3 -D -e main -Od -S " + phase + " " + hlslShader.string() + " -o " + outputShader.string() + " > " + logFile.string();

			spdlog::info("comnmand: {0}", command);
			int status = system(command.c_str());

			if (status != 0) {
				std::ifstream file(logFile.string());
				std::string line;

				spdlog::error("fail to compile shader:");
				while (std::getline(file, line)) {
					spdlog::error(line);
				}
			}
		}

		m_compiledShaderCode = readFile(outputShader.string());
		m_compiledShaderPath = outputShader.string();
	}
}
