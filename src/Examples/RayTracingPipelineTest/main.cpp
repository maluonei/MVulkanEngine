#include "test.hpp"
#include "Managers/singleton.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

int main() {
	RayTracingPipelineTest test;
	test.Init();
	
	test.Run();
	test.Clean();
}
