#include "RayQueryShadows.hpp"
#include "Managers/singleton.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

int main() {
	RayQueryPbr test;
	test.Init();

	test.Run();
	test.Clean();
}
