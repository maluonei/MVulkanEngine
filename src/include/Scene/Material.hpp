#pragma once
#ifndef MATERIAL_H	
#define MATERIAL_H

#include "glm/glm.hpp"
#include "string"


struct PhongMaterial {
	glm::vec3 diffuseColor;
	std::string diffuseTexture = "";
};




#endif