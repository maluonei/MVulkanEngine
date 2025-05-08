#include "Frustum.hpp"

//static glm::vec4 GetPlane(glm::vec3 normal, glm::vec3 point) {
//	return glm::vec4(normal.x, normal.y, normal.z, -glm::dot(normal, point));
//};
//
//static glm::vec4 GetPlane(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
//	glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
//	return GetPlane(normal, a);
//};