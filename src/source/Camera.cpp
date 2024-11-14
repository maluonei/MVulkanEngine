#include"Camera.hpp"
#include"Util.hpp"
#include <spdlog/spdlog.h>

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

void Camera::Move(Direction direction, float scale)
{
	switch (direction) {
	case Direction::Forward:
		position += scale * forward;
		break;
	case Direction::Right:
		position += scale * right;
		break;
	case Direction::Up:
		position += scale * up;
		break;
	}
}

void Camera::Rotate(float pitch, float yaw)
{
	pitch /= 500.f;
	yaw /= 500.f;

	float currentPitch, currentYaw;

	//spdlog::info("direction:" + std::to_string(direction.x) + "," + std::to_string(direction.y) + "," + std::to_string(direction.z));
	//spdlog::info("before direction:" + std::to_string(direction.x) + "," + std::to_string(direction.y) + "," + std::to_string(direction.z));
	DirectionToPitchYaw(direction, currentPitch, currentYaw);

	//spdlog::info("currentPitch:" + std::to_string(currentPitch) + ", currentYaw" + std::to_string(currentYaw));
	currentPitch+=pitch;
	currentYaw-=yaw;

	currentYaw = glm::clamp(currentYaw, -glm::pi<float>() / 2.f, glm::pi<float>() / 2.f);

	PitchYawToDirection(currentPitch, currentYaw, direction);
	//spdlog::info("after direction:" + std::to_string(direction.x) + "," + std::to_string(direction.y) + "," + std::to_string(direction.z));

	forward = glm::normalize(direction);
	right = glm::normalize(glm::cross(forward, UP));
	up = glm::normalize(glm::cross(right, forward));
}
