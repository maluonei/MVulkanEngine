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

struct BoundingBox {
    glm::vec3 pMin;
    glm::vec3 pMax;

    const inline glm::vec3 GetCenter() const {
        return (pMax + pMin) / 2.0f;
    }
};

void DirectionToPitchYaw(glm::vec3 direction, float& pitch, float& yaw);
void PitchYawToDirection(float pitch, float yaw, glm::vec3& direction);

std::ostream& operator<<(std::ostream& os, const glm::mat4& mat);

std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);


#endif