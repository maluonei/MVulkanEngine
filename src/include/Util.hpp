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

void DirectionToPitchYaw(glm::vec3 direction, float& pitch, float& yaw);
void PitchYawToDirection(float pitch, float yaw, glm::vec3& direction);


#endif