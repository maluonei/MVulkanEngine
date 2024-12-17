#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/ext/matrix_clip_space.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <math.h>

//static const glm::vec3 UP = { 0.f, 1.f, 0.f };

class Frustum;

enum Direction {
	Right, Up, Forward
};

class Camera {
private:
	bool useOrtho = false;

	glm::mat4 projMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 orthoMatrix;
	//Frustum frustum;

	void UpdateViewMatrix() {
		viewMatrix = glm::mat4();
		viewMatrix = glm::lookAt(position, position+direction, UP);
	}

public:
	float fov;
	float aspect_ratio;
	float zNear;
	float zFar;

	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 UP = { 0.f, 1.f, 0.f };

	glm::vec3 forward;
	glm::vec3 up;
	glm::vec3 right;

	Camera(glm::vec3 _position, glm::vec3 _direction, float _fov, float _aspect_ratio, float _zNear, float _zFar);
	Camera(glm::vec3 _position, glm::vec3 _direction, glm::vec3 _up, float _fov, float _aspect_ratio, float _zNear, float _zFar);

	inline float GetZnear()const { return zNear; }
	inline float GetZfar()const { return zFar; }

	inline void SetOrth(bool _flag) {
		useOrtho = true;
	}

	inline glm::vec3 GetPosition() const {
		return position;
	}

	inline glm::vec3 GetDirection() const {
		return direction;
	}

	inline glm::mat4 GetProjMatrix()const {
		if (IsOrtho()) return orthoMatrix;
		else return projMatrix;
	}

	inline glm::mat4 GetViewMatrix() {
		UpdateViewMatrix();
		return viewMatrix;
	}

	inline glm::mat4 GetOrthoMatrix() const {
		return orthoMatrix;
	}

	inline glm::mat4* GetProjMatrixPtr() {
		if (IsOrtho()) return &orthoMatrix;
		else return &projMatrix;
	}

	inline glm::mat4* GetViewMatrixPtr() {
		return &viewMatrix;
	}

	inline bool IsOrtho() const {
		return useOrtho;
	}

	inline void SetOrtho(bool flag) {
		useOrtho = flag;
	}

	inline glm::vec3 GetUp() {
		return up;
	}

	inline glm::vec3 GetRight() {
		return right;
	}

	void Move(Direction direction, float scale);

	void Rotate(float pitch, float yaw);
};


#endif