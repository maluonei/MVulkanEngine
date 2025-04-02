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

FrustumData Camera::GetFrustumData()
{
	FrustumData data;

	data.Cone = GetFrustumCone();
	data.Sphere = GetFrustumSphere();
	GetFrustumPlane(
		&data.Planes[0],
		&data.Planes[1],
		&data.Planes[2],
		&data.Planes[3],
		&data.Planes[4],
		&data.Planes[5]
	);

	return data;
}

FrustumCone Camera::GetFrustumCone()
{
	FrustumCone cone;

	cone.Tip = GetPosition();
	cone.Direction = GetDirection();
	cone.Height = GetZfar();
	cone.Angle = glm::radians(GetFov());

	bool fitFarClip = false;
	if (fitFarClip) {
		// View projection matrix
		auto VP = GetProjMatrix() * GetViewMatrix();
		// Inverse view projection matrix
		auto  invVP = glm::inverse(VP);
		// Clip space coordinates
		auto csFarTL = glm::vec3(-1, 1, 1);
		auto csFarBL = glm::vec3(-1, -1, 1);
		auto csFarBR = glm::vec3(1, -1, 1);
		auto csFarTR = glm::vec3(1, 1, 1);
		// Transform into view coordinates using inverse view projection matrix
		auto farTL = invVP * glm::vec4(csFarTL, 1.0f);
		auto farBL = invVP * glm::vec4(csFarBL, 1.0f);
		auto farBR = invVP * glm::vec4(csFarBR, 1.0f);
		auto farTR = invVP * glm::vec4(csFarTR, 1.0f);
		// Divide to finalize unproject
		farTL /= farTL.w;
		farBL /= farBL.w;
		farBR /= farBR.w;
		farTR /= farTR.w;
		// Find center of far clip plane
		auto farCenter = (farTL + farBL + farBR + farTR) / 4.0f;
		// Distance from far clip plane center to top left corner of far clip plane
		float r = glm::distance(farCenter, farTL);
		// Calculate angle using arctan
		cone.Angle = 2.0f * atan(r / GetZfar());
	}
	return cone;

}

glm::vec4 Camera::GetFrustumSphere() 
{
	// View projection matrix
	auto VP = GetProjMatrix() * GetViewMatrix();
	// Inverse view projection matrix
	auto  invVP = glm::inverse(VP);

	auto csNearTL = glm::vec3(-1, 1, -1);
	auto csNearBL = glm::vec3(-1, -1, -1);
	auto csNearBR = glm::vec3(1, -1, -1);
	auto csNearTR = glm::vec3(1, 1, -1);

	auto csFarTL = glm::vec3(-1, 1, 1);
	auto csFarBL = glm::vec3(-1, -1, 1);
	auto csFarBR = glm::vec3(1, -1, 1);
	auto csFarTR = glm::vec3(1, 1, 1);

	auto nearTL = invVP * glm::vec4(csNearTL, 1.0f);
	auto nearBL = invVP * glm::vec4(csNearBL, 1.0f);
	auto nearBR = invVP * glm::vec4(csNearBR, 1.0f);
	auto nearTR = invVP * glm::vec4(csNearTR, 1.0f);

	auto farTL = invVP * glm::vec4(csFarTL, 1.0f);
	auto farBL = invVP * glm::vec4(csFarBL, 1.0f);
	auto farBR = invVP * glm::vec4(csFarBR, 1.0f);
	auto farTR = invVP * glm::vec4(csFarTR, 1.0f);

	nearTL /= nearTL.w;
	nearBL /= nearBL.w;
	nearBR /= nearBR.w;
	nearTR /= nearTR.w;

	farTL /= farTL.w;
	farBL /= farBL.w;
	farBR /= farBR.w;
	farTR /= farTR.w;

	auto nearCenter = (nearTL + nearBL + nearBR + nearTR) / 4.0f;
	auto farCenter = (farTL + farBL + farBR + farTR) / 4.0f;
	auto center = glm::vec3(nearCenter + farCenter) / 2.0f;

	float r = glm::distance(center, glm::vec3(nearTL));
	r = std::max(r, glm::distance(center, glm::vec3(nearBL)));
	r = std::max(r, glm::distance(center, glm::vec3(nearBR)));
	r = std::max(r, glm::distance(center, glm::vec3(nearTR)));
	r = std::max(r, glm::distance(center, glm::vec3(farTL)));
	r = std::max(r, glm::distance(center, glm::vec3(farBL)));
	r = std::max(r, glm::distance(center, glm::vec3(farBR)));
	r = std::max(r, glm::distance(center, glm::vec3(farTR)));

	return glm::vec4(center, r);
}

void Camera::GetFrustumPlane(FrustumPlane* pLeft, FrustumPlane* pRight, FrustumPlane* pTop, FrustumPlane* pBottom, FrustumPlane* pNear, FrustumPlane* pFar)
{
	// View projection matrix
	auto VP = GetProjMatrix() * GetViewMatrix();
	// Inverse view projection matrix
	auto  invVP = glm::inverse(VP);

	auto csNearTL = glm::vec3(-1, 1, -1);
	auto csNearBL = glm::vec3(-1, -1, -1);
	auto csNearBR = glm::vec3(1, -1, -1);
	auto csNearTR = glm::vec3(1, 1, -1);

	auto csFarTL = glm::vec3(-1, 1, 1);
	auto csFarBL = glm::vec3(-1, -1, 1);
	auto csFarBR = glm::vec3(1, -1, 1);
	auto csFarTR = glm::vec3(1, 1, 1);

	auto nearTL = invVP * glm::vec4(csNearTL, 1.0f);
	auto nearBL = invVP * glm::vec4(csNearBL, 1.0f);
	auto nearBR = invVP * glm::vec4(csNearBR, 1.0f);
	auto nearTR = invVP * glm::vec4(csNearTR, 1.0f);

	auto farTL = invVP * glm::vec4(csFarTL, 1.0f);
	auto farBL = invVP * glm::vec4(csFarBL, 1.0f);
	auto farBR = invVP * glm::vec4(csFarBR, 1.0f);
	auto farTR = invVP * glm::vec4(csFarTR, 1.0f);

	nearTL /= nearTL.w;
	nearBL /= nearBL.w;
	nearBR /= nearBR.w;
	nearTR /= nearTR.w;

	farTL /= farTL.w;
	farBL /= farBL.w;
	farBR /= farBR.w;
	farTR /= farTR.w;

	if (pLeft != nullptr)
	{
		auto nearH = glm::vec3(nearTL + nearBL) / 2.0f;
		auto farH = glm::vec3(farTL + farBL) / 2.0f;
		auto u = glm::normalize(farH - nearH);
		auto v = glm::normalize(glm::vec3(nearTL - nearBL));
		auto w = glm::cross(u, v);
		w = glm::normalize(w);

		pLeft->Normal = w;
		pLeft->Position = (nearH + farH) / 2.0f;

		pLeft->C0 = farTL;
		pLeft->C1 = farBL;
		pLeft->C2 = nearBL;
		pLeft->C3 = nearTL;
	}

	if (pRight != nullptr)
	{
		auto nearH = glm::vec3(nearTR + nearBR) / 2.0f;
		auto farH = glm::vec3(farTR + farBR) / 2.0f;
		auto u = glm::normalize(farH - nearH);
		auto v = glm::normalize(glm::vec3(nearBL - nearTL));
		auto w = glm::cross(u, v);
		w = glm::normalize(w);

		pRight->Normal = w;
		pRight->Position = (nearH + farH) / 2.0f;

		pRight->C0 = nearTR;
		pRight->C1 = nearBR;
		pRight->C2 = farBR;
		pRight->C3 = farTR;
	}

	if (pTop != nullptr)
	{
		auto nearH = glm::vec3(nearTL + nearTR) / 2.0f;
		auto farH = glm::vec3(farTL + farTR) / 2.0f;
		auto u = glm::normalize(farH - nearH);
		auto v = glm::normalize(glm::vec3(nearTR - nearTL));
		auto w = glm::cross(u, v);
		w = glm::normalize(w);

		pTop->Normal = w;
		pTop->Position = (nearH + farH) / 2.0f;

		pTop->C0 = farTL;
		pTop->C1 = nearTL;
		pTop->C2 = nearTR;
		pTop->C3 = farTR;
	}

	if (pBottom != nullptr)
	{
		auto nearH = glm::vec3(nearBL + nearBR) / 2.0f;
		auto farH = glm::vec3(farBL + farBR) / 2.0f;
		auto u = glm::normalize(farH - nearH);
		auto v = glm::normalize(glm::vec3(nearBL - nearBR));
		auto w = glm::cross(u, v);
		w = glm::normalize(w);

		pBottom->Normal = w;
		pBottom->Position = (nearH + farH) / 2.0f;

		pBottom->C0 = nearBL;
		pBottom->C1 = farBL;
		pBottom->C2 = farBR;
		pBottom->C3 = nearBR;
	}

	if (pNear != nullptr)
	{
		pNear->Normal = GetDirection();
		pNear->Position = (nearTL + nearBR) / 2.0f;

		pNear->C0 = nearTL;
		pNear->C1 = nearBL;
		pNear->C2 = nearBR;
		pNear->C3 = nearTR;
	}

	if (pFar != nullptr)
	{
		pFar->Normal = -GetDirection();
		pFar->Position = (farTL + farBR) / 2.0f;

		pFar->C0 = farTL;
		pFar->C1 = farBL;
		pFar->C2 = farBR;
		pFar->C3 = farTR;
	}
}

