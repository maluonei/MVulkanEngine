#include"Camera.hpp"

Camera::Camera(glm::vec3 _position, glm::vec3 _direction, float _fov, float _aspect_ratio, float _zNear, float _zFar)
	:fov(_fov), aspect_ratio(_aspect_ratio), zNear(_zNear), zFar(_zFar), position(_position), direction(_direction)
{
	forward = glm::normalize(_direction);
	right = glm::normalize(glm::cross(forward, UP));
	up = glm::normalize(glm::cross(right, forward));

	projMatrix = glm::perspective(glm::radians(fov), aspect_ratio, zNear, zFar);
	orthoMatrix = glm::ortho(-10.f, 10.f, -7.f, 7.f, 0.1f, 1000.f);
	UpdateViewMatrix();
}