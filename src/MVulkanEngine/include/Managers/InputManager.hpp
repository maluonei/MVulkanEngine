#pragma once
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "Singleton.hpp"
#include "glm/glm.hpp"

class InputManager : public Singleton<InputManager> {
public:
	void DealInputs();

	inline void SetKey(int key, bool pressed) { Keys[key] = pressed; }
	inline void SetMousePosition(glm::vec2 pos){ 
		mousePos = pos; 
	}
	inline void SetCursorEnter(bool entered) { cursorEnter = entered; }
	inline void SetCursorLeave(bool leaved) { cursorLeave = leaved; }
	inline void SetMousePressed(int button, bool pressed){ mouseButtons[button] = pressed; }

private:
	void DealMouseMoveInput();
	void DealMouseClickInput();
	void DealKeyboardInput();
	void DealCursorEnter();

	bool         Keys[1024];
	bool         LastPressed[1024];
	unsigned int Width, Height;
	bool cursorEnter = false;
	bool cursorLeave = false;
	bool valid = true;
	bool KeysProcessed[1024];
	bool mouseButtons[4];
	bool rotateDirections[4];
	//glm::vec2 dMousePos{};
	glm::vec2 mousePos{};
	glm::vec2 previousPos{};
};


#endif // INPUTMANAGER_H