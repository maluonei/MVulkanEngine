#include "sdfgi.hpp"
#include "Managers/singleton.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

int main() {
	SDFGIApplication sdfgi;
	sdfgi.Init();

	sdfgi.Run();
	sdfgi.Clean();
}

