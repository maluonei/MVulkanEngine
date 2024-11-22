#pragma once
#ifndef POINTLIGHT_HPP
#define POINTLIGHT_HPP

#include "Light.hpp"

class PointLight : public Light {
public:
	PointLight(){}
	PointLight(glm::vec3 position, glm::vec3 color, float intensity) : Light(color, intensity, LightType::POINT), m_position(position) {}

	glm::vec3 GetPosition() const { return m_position; }
	void SetPosition(glm::vec3 position) { m_position = position; }
private:
	glm::vec3 m_position;
};


#endif

