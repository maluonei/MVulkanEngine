#include "voxel.hpp"
#include "Managers/singleton.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

int main() {
	VOXEL voxel;
	voxel.Init();

	voxel.Run();
	voxel.Clean();
}

