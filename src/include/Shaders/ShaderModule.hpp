#ifndef SHADERMODULE_H
#define SHADERMODULE_H

#include<glm/glm.hpp>
#include<string>
#include<unordered_map>


class ShaderModule {
public:
	

private:


};

class TestFrag : public ShaderModule {
private:
	struct UniformBufferObject {
		glm::vec3 color0;
		float padding0;
		glm::vec3 color1;
		float padding1;
	} ubo;

	std::unordered_map<std::string, uint32_t> bindings;

public:
	TestFrag();
	void SetUBO(glm::vec3 color0, glm::vec3 color1);
	inline size_t GetBufferSize() const { return sizeof(ubo); }
	inline void* GetData() { return &ubo; }
};



#endif