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

	glm::mat4 m_projMatrix;
	glm::mat4 m_viewMatrix;
	glm::mat4 m_orthoMatrix;
	//Frustum frustum;

	float m_fov;
	float m_aspect_ratio;
	float m_zNear;
	float m_zFar;

	glm::vec3 m_position;
	glm::vec3 m_direction;
	glm::vec3 m_UP = { 0.f, 1.f, 0.f };

	glm::vec3 m_forward;
	glm::vec3 m_up;
	glm::vec3 m_right;

	void UpdateViewMatrix() {
		m_viewMatrix = glm::mat4();
		m_viewMatrix = glm::lookAt(m_position, m_position+ m_direction, m_UP);
	}

public:

	Camera(glm::vec3 _position, glm::vec3 _direction, float _fov, float _aspect_ratio, float _zNear, float _zFar);
	Camera(glm::vec3 _position, glm::vec3 _direction, glm::vec3 _up, float _fov, float _aspect_ratio, float _zNear, float _zFar);

	inline float GetZnear()const { return m_zNear; }
	inline float GetZfar()const { return m_zFar; }

	inline float GetFov() const {
		return m_fov;
	}

	inline void SetOrth(bool _flag) {
		useOrtho = true;
	}

	inline glm::vec3 GetPosition() const {
		return m_position;
	}

	inline glm::vec3 GetDirection() const {
		return m_direction;
	}

	inline glm::mat4 GetProjMatrix()const {
		if (IsOrtho()) return m_orthoMatrix;
		else return m_projMatrix;
	}

	inline glm::mat4 GetViewMatrix() {
		UpdateViewMatrix();
		return m_viewMatrix;
	}

	inline glm::mat4 GetOrthoMatrix() const {
		return m_orthoMatrix;
	}

	inline glm::mat4* GetProjMatrixPtr() {
		if (IsOrtho()) return &m_orthoMatrix;
		else return &m_projMatrix;
	}

	inline glm::mat4* GetViewMatrixPtr() {
		return &m_viewMatrix;
	}

	inline bool IsOrtho() const {
		return useOrtho;
	}

	inline void SetOrtho(bool flag) {
		useOrtho = flag;
	}

	inline glm::vec3 GetUp() {
		return m_up;
	}

	inline glm::vec3 GetRight() {
		return m_right;
	}

	void Move(Direction direction, float scale);

	void Rotate(float pitch, float yaw);
};


#endif