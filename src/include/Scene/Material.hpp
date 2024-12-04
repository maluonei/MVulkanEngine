#pragma once
#ifndef MATERIAL_H	
#define MATERIAL_H

#include "glm/glm.hpp"
#include "string"


struct PhongMaterial {
	glm::vec3 diffuseColor;
	std::string diffuseTexture = "";

	glm::vec3 specularColor;
	std::string specularTexture = "";

	std::string normalMap = "";

	float roughness = 0.5f;
	float metallic = 0.0f;
	std::string metallicAndRoughnessTexture = "";
};




#endif