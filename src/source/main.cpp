#include "MVulkanRHI/MVulkanEngine.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

int main() {
    MVulkanEngine engine;
    engine.SetWindowRes(WIDTH, HEIGHT);
    engine.Init();
    engine.Run();
}
