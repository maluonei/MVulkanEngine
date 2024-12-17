#include "Shaders/Shader.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

Shader::Shader()
{
}


Shader::Shader(std::string _path, ShaderStageFlagBits _stage)
{
	shaderPath = _path;
	shaderStage = _stage;
}

void Shader::Compile() {
	compile(shaderPath);
}

void Shader::compile(std::string shaderPath)
{
	fs::path projectRootPath = PROJECT_ROOT;
	fs::path shaderRootPath = projectRootPath / "resources" / "shaders";

	fs::path compilerPath = projectRootPath / "build" / "ShaderCompilers" / "glslangValidator.exe";

	spdlog::info("compilng shader: {0}", shaderPath);

	if (shaderPath.ends_with("glsl")) {
		size_t size = shaderPath.size();

		fs::path glslShader = shaderRootPath / (shaderPath);
		fs::path outputShader = shaderRootPath / (shaderPath.substr(0, size - 5) + ".spv");

		if (!fs::exists(glslShader)) {
			spdlog::error("source shader don't exist: {0}", shaderPath);
		}

		fs::path logFile = shaderRootPath / "log.txt";
		//std::string shaderName = shaderPath.substr(0, shaderPath.size() - 5);
		std::string command = compilerPath.string() +  " -V " + glslShader.string() + " -o " + outputShader.string() + " > " + logFile.string();
		int status = system(command.c_str());

		if (status != 0) {
			std::ifstream file(logFile.string());
			std::string line;

			spdlog::error("fail to compile shader:");
			while (std::getline(file, line)) {
				spdlog::error(line);
			}
		}

		compiledShaderCode = readFile(outputShader.string());
		compiledShaderPath = outputShader.string();
	}
	else if (shaderPath.ends_with("hlsl")) {
		size_t size = shaderPath.size();

		fs::path glslShader = shaderRootPath / (shaderPath);
		fs::path outputShader = shaderRootPath / (shaderPath.substr(0, size - 5) + ".spv");

		if (!fs::exists(glslShader)) {
			spdlog::error("source shader don't exist: {0}", shaderPath);
		}

		std::string phase = "";
		if (shaderPath.substr(size - 9, 4) == "vert") {
			phase = "vert";
		}
		else if (shaderPath.substr(size - 9, 4) == "frag") {
			phase = "frag";
		}
		else if (shaderPath.substr(size - 9, 4) == "geom") {
			phase = "geom";
		}
		//spdlog::info("phase: {0}", phase);

		fs::path logFile = shaderRootPath / "log.txt";
		std::string command = compilerPath.string() + " -V -D -e main -Od -S " + phase + " " + glslShader.string() + " -o " + outputShader.string() + " --keep-uncalled > " + logFile.string();
		int status = system(command.c_str());

		//spdlog::info("command:, {0}", command);

		if (status != 0) {
			std::ifstream file(logFile.string());
			std::string line;

			spdlog::error("fail to compile shader:");
			while (std::getline(file, line)) {
				spdlog::error(line);
			}
		}

		compiledShaderCode = readFile(outputShader.string());
		compiledShaderPath = outputShader.string();
	}
}
