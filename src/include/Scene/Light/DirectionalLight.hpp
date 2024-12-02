#pragma once
#ifndef DERACTIONALLIGHT_HPP
#define DERACTIONALLIGHT_HPP

#include "Light.hpp"

class DirectionalLight : public Light {
public:
	DirectionalLight(){}
	DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity) : Light(color, intensity, LightType::DIRECTIONAL), m_direction(direction) {}

	glm::vec3 GetDirection() const { return m_direction; }
	void SetDirection(glm::vec3 direction) { m_direction = direction; }

	//virtual std::shared_ptr<Camera> GetLightCamera(BoundingBox bbx);
private:
	glm::vec3 m_direction;
};


#endif