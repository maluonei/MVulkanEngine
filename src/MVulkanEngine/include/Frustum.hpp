#ifndef FRUSTRUM_HPP
#define FRUSTRUM_HPP

#include <glm/glm.hpp>

enum class MFrustumPlane : int {
    MNEAR = 0,
    MFAR = 1,
    MLEFT = 2,
    MRIGHT = 3,
    MUP = 4,
    MBOTTOM = 5,
};

struct Frustum
{
    glm::vec4 planes[6];
};

static glm::vec4 GetPlane(glm::vec3 normal, glm::vec3 point) {
    return glm::vec4(normal.x, normal.y, normal.z, -glm::dot(normal, point));
};

static glm::vec4 GetPlane(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    return GetPlane(normal, a);
};

#endif