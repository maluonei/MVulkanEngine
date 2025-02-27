#include "ddgiPass.hpp"
#include "Managers/singleton.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

int main() {
	DDGIApplication ddgi;
	ddgi.Init();

	ddgi.Run();
	ddgi.Clean();
}

