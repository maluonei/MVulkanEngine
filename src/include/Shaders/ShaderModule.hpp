#ifndef SHADERMODULE_H
#define SHADERMODULE_H

#include<glm/glm.hpp>
#include<string>
#include<unordered_map>


class ShaderModule {
public:
	

private:


};

class TestShader : public ShaderModule {
private:
	struct UniformBufferObjectVert {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	} ubo0;

	struct UniformBufferObjectFrag {
		glm::vec3 color0;
		float padding0;
		glm::vec3 color1;
		float padding1;
	} ubo1;

	std::vector<uint32_t> sizes;

public:
	TestShader();
	void SetUBO0(glm::mat4 model, glm::mat4 view, glm::mat4 projection);
	void SetUBO1(glm::vec3 color0, glm::vec3 color1);
	inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizes[binding]; }
	void* GetData(uint32_t binding);
};



#endif