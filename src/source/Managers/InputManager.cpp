#include <glfw/glfw3.h>
#include "Managers/InputManager.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Camera.hpp"
#include "spdlog/spdlog.h"


void InputManager::DealMouseMoveInput()
{
	if (!cursorEnter) {
		glm::vec2 dMousePos = mousePos - previousPos;

		Singleton<MVulkanEngine>::instance().GetCamera()->Rotate(dMousePos.x, dMousePos.y);
	}
	previousPos = mousePos;
}

void InputManager::DealMouseClickInput()
{

}

void InputManager::DealKeyboardInput()
{
	float velocity = 0.1f;

	if (Keys[GLFW_KEY_W]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Up, velocity);
	}
	if (Keys[GLFW_KEY_S]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Up, -velocity);
	}
	if (Keys[GLFW_KEY_A]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Right, -velocity);
	}
	if (Keys[GLFW_KEY_D]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Right, velocity);
	}
	if (Keys[GLFW_KEY_Q]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Forward, velocity);
	}
	if (Keys[GLFW_KEY_E]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Forward, -velocity);
	}
}

void InputManager::DealInputs()
{
	//spdlog::info("InputManager::DealInputs()");
	DealCursorEnter();
	if (valid) {
		DealMouseMoveInput();
		DealMouseClickInput();
		DealKeyboardInput();
	}

	cursorEnter = false;
	cursorLeave = false;
}

void InputManager::DealCursorEnter() {
	if (cursorEnter) {
		valid = true;
	}

	if (cursorLeave) {
		valid = false;
	}

}
