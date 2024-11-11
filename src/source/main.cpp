#include "MVulkanRHI/MVulkanEngine.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

int main() {
    Singleton<MVulkanEngine>::instance().SetWindowRes(WIDTH, HEIGHT);
    Singleton<MVulkanEngine>::instance().Init();
    Singleton<MVulkanEngine>::instance().Run();
}
