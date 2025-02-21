#include <glfw/glfw3.h>
#include "Managers/InputManager.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Camera.hpp"
#include "spdlog/spdlog.h"
#include "MRenderApplication.hpp"


void InputManager::RegisterApplication(MRenderApplication* _application)
{
	m_application = _application;
}

//void InputManager::RegisterSetCameraMoveFunc(MRenderApplication::SetCameraMove func)
//{
//	m_setCameraMoved = func;
//}

void InputManager::DealMouseMoveInput()
{
	if (!cursorEnter) {
		glm::vec2 dMousePos = mousePos - previousPos;

		if (Singleton<MVulkanEngine>::instance().GetCamera() != nullptr) {
			Singleton<MVulkanEngine>::instance().GetCamera()->Rotate(dMousePos.x, dMousePos.y);
		}

		if(m_application)
			if(dMousePos!=glm::vec2(0.f, 0.f))
				m_application->SetCameraMoved(true);
		//m_setCameraMoved(true);
	}
	previousPos = mousePos;
}

void InputManager::DealMouseClickInput()
{

}

void InputManager::DealKeyboardInput()
{
	float velocity = 0.01f;

	if (Keys[GLFW_KEY_W]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Up, velocity);
		m_application->SetCameraMoved(true);
	}
	if (Keys[GLFW_KEY_S]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Up, -velocity);
		m_application->SetCameraMoved(true);
	}
	if (Keys[GLFW_KEY_A]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Right, -velocity);
		m_application->SetCameraMoved(true);
	}
	if (Keys[GLFW_KEY_D]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Right, velocity);
		m_application->SetCameraMoved(true);
	}
	if (Keys[GLFW_KEY_Q]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Forward, velocity);
		m_application->SetCameraMoved(true);
	}
	if (Keys[GLFW_KEY_E]) {
		Singleton<MVulkanEngine>::instance().GetCamera()->Move(Direction::Forward, -velocity);
		m_application->SetCameraMoved(true);
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
