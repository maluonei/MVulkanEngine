#ifndef SHADERMODULE_H
#define SHADERMODULE_H

#include"glm/glm.hpp"

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

public:
	void SetUBO(glm::vec3 color0, glm::vec3 color1);
};



#endif