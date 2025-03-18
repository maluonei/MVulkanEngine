#include"Camera.hpp"
#include"Util.hpp"
#include <spdlog/spdlog.h>

Camera::Camera(glm::vec3 _position, glm::vec3 _direction, float _fov, float _aspect_ratio, float _zNear, float _zFar)
	:m_fov(_fov), m_aspect_ratio(_aspect_ratio), m_zNear(_zNear), m_zFar(_zFar), m_position(_position), m_direction(_direction)
{
	m_forward = glm::normalize(_direction);
	m_right = glm::normalize(glm::cross(m_forward, m_UP));
	m_up = glm::normalize(glm::cross(m_right, m_forward));

	m_projMatrix = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_zNear, m_zFar);
	m_orthoMatrix = glm::ortho(-20.f, 20.f, -20.f, 20.f, m_zNear, m_zFar);
	//orthoMatrix = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.001f, 10000.f);
	UpdateViewMatrix();
}

Camera::Camera(glm::vec3 _position, glm::vec3 _direction, glm::vec3 _up, float _fov, float _aspect_ratio, float _zNear, float _zFar)
	:m_fov(_fov), m_aspect_ratio(_aspect_ratio), m_zNear(_zNear), m_zFar(_zFar), m_position(_position), m_direction(_direction), m_UP(_up)
{
	m_forward = glm::normalize(_direction);
	m_right = glm::normalize(glm::cross(m_forward, m_UP));
	m_up = glm::normalize(glm::cross(m_right, m_forward));

	m_projMatrix = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_zNear, m_zFar);
	m_orthoMatrix = glm::ortho(-20.f, 20.f, -20.f, 20.f, m_zNear, m_zFar);
	//orthoMatrix = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.001f, 10000.f);
	UpdateViewMatrix();
}

Camera::Camera(glm::vec3 _position, glm::vec3 _direction, glm::vec3 _up, float _aspect_ratio, float _xMin, float _xMax, float _yMin, float _yMax, float _zNear, float _zFar)
	:m_fov(60.f), m_aspect_ratio(_aspect_ratio), m_zNear(_zNear), m_zFar(_zFar), m_position(_position), m_direction(_direction), m_UP(_up)
{
	m_forward = glm::normalize(_direction);
	m_right = glm::normalize(glm::cross(m_forward, m_UP));
	m_up = glm::normalize(glm::cross(m_right, m_forward));

	//m_projMatrix = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_zNear, m_zFar);
	m_orthoMatrix = glm::ortho(_xMin, _xMax, _yMin, _yMax, _zNear, _zFar);
	//orthoMatrix = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.001f, 10000.f);
	UpdateViewMatrix();
}

void Camera::Move(Direction direction, float scale)
{
	switch (direction) {
	case Direction::Forward:
		m_position += scale * m_forward;
		break;
	case Direction::Right:
		m_position += scale * m_right;
		break;
	case Direction::Up:
		m_position += scale * m_up;
		break;
	}
}

void Camera::Rotate(float pitch, float yaw)
{
	pitch /= 500.f;
	yaw /= 500.f;

	float currentPitch, currentYaw;

	DirectionToPitchYaw(m_direction, currentPitch, currentYaw);

	currentPitch+=pitch;
	currentYaw-=yaw;

	currentYaw = glm::clamp(currentYaw, -glm::pi<float>() / 2.f, glm::pi<float>() / 2.f);

	PitchYawToDirection(currentPitch, currentYaw, m_direction);

	m_forward = glm::normalize(m_direction);
	m_right = glm::normalize(glm::cross(m_forward, m_UP));
	m_up = glm::normalize(glm::cross(m_right, m_forward));
}
