#include "Shaders/ShaderModule.hpp"

TestFrag::TestFrag()
{
	
}

void TestFrag::SetUBO(glm::vec3 color0, glm::vec3 color1)
{
	ubo.color0 = color0;
	ubo.color1 = color1;
}
