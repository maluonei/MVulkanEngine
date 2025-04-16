#pragma once
#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include "glm/glm.hpp"

#define PI 3.1415926535f

struct BoundingBox {
    glm::vec3 pMin;
    glm::vec3 pMax;
    BoundingBox() = default;
    BoundingBox(glm::vec3 inPmin, glm::vec3 inPmax);
    BoundingBox(const BoundingBox& bbx) = default;

    const glm::vec3 GetCenter() const;
    const glm::vec3 GetExtent() const;
    const glm::vec3 GetSize() const;
    const BoundingBox ExpandBy(const glm::vec3 scale) const;
};

void DirectionToPitchYaw(glm::vec3 direction, float& pitch, float& yaw);
void PitchYawToDirection(float pitch, float yaw, glm::vec3& direction);

std::ostream& operator<<(std::ostream& os, const glm::mat4& mat);

std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);

glm::vec4 RotationMatrixToQuaternion(glm::mat3 R);



#endif