#include "rtao.hpp"
#include "Managers/singleton.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

int main() {
	RTAO rtao;
	rtao.Init();

	rtao.Run();
	rtao.Clean();
}

