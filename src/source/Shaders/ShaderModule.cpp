#include "Shaders/ShaderModule.hpp"

TestShader::TestShader()
{
	sizes.resize(3);
	sizes[0] = sizeof(ubo0);
	sizes[1] = sizeof(ubo1);
	sizes[2] = 0;
}

void TestShader::SetUBO0(glm::mat4 model, glm::mat4 view, glm::mat4 projection)
{
	ubo0.Model = model;
	ubo0.View = view;
	ubo0.Projection = projection;
}

void* TestShader::GetData(uint32_t binding)
{
	switch (binding) {
	case 0:
		return &ubo0;
	case 1:
		return &ubo1;
	case 2:
	default:
		return nullptr;
	}
}

void TestShader::SetUBO1(glm::vec3 color0, glm::vec3 color1)
{
	ubo1.color0 = color0;
	ubo1.color1 = color1;
}
