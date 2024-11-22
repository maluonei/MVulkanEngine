#pragma once
#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "glm/glm.hpp"

enum class LightType : uint8_t {
	NONE = 0,
	DIRECTIONAL,
	POINT,
	SPOT
};

class Light {
public:
	Light()
		:m_color(glm::vec3(0.f)), m_intensity(1.f), m_type(LightType::NONE){}
	Light(glm::vec3 color, float intensity, LightType type)
		:m_color(color), m_intensity(intensity), m_type(type) {}

	inline glm::vec3 GetColor() const { return m_color; }
	inline void SetColor(glm::vec3 color) { m_color = color; }
	inline float GetIntensity() const { return m_intensity; }
	inline void SetIntensity(float intensity) { m_intensity = intensity; }
	inline LightType GetType() const { return m_type; }

private:
	glm::vec3 m_color;
	float m_intensity;
	LightType m_type;
};

#endif // !
