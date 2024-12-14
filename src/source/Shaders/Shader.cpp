#include "Shaders/Shader.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
//std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& sourceCode, std::string inputFileName, shaderc_shader_kind kind) {
//	// 创建编译器对象
//	shaderc::Compiler compiler;
//	shaderc::CompileOptions options;
//
//	// 编译 GLSL 为 SPIR-V
//	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(sourceCode, kind, inputFileName.c_str(), options);
//
//	// 检查编译状态
//	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
//		std::cerr << "Shader compilation failed: " << module.GetErrorMessage() << std::endl;
//		return {};
//	}
//
//	// 将 SPIR-V 字节码存储到 vector 中并返回
//	return { module.cbegin(), module.cend() };
//}
//
//Shader::Shader(std::string vertPath, std::string fragPath)
//{
//	std::vector<unsigned int> vertShaderCodeSpirV;
//	std::vector<unsigned int> fragShaderCodeSpirV;
//
//	std::string vertShaderCode = readFileToString(vertPath);
//	std::string fragShaderCode = readFileToString(fragPath);
//
//	std::vector<unsigned int> vertShaderCodeSpirv;
//	std::vector<unsigned int> fragShaderCodeSpirv;
//
//	auto vertSpirv = CompileGLSLToSPIRV(vertShaderCode, vertPath, shaderc_vertex_shader);
//	auto fragSpirv = CompileGLSLToSPIRV(fragShaderCode, fragPath, shaderc_fragment_shader);
//	
//}
//

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
	fs::path shaderRootPath = fs::current_path().parent_path().parent_path().append("resources").append("shaders");
	//spdlog::info("shaderRootPath:", shaderRootPath.string());
	fs::path compilerPath = fs::current_path().parent_path().append("ShaderCompilers").append("glslangValidator.exe");

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
