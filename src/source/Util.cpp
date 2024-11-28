#include "Util.hpp"

void DirectionToPitchYaw(glm::vec3 direction, float& pitch, float& yaw)
{
    yaw = atan2(direction.y, sqrt(direction.x * direction.x + direction.z * direction.z));
    pitch = atan2(direction.z, direction.x);
}

void PitchYawToDirection(float pitch, float yaw, glm::vec3& direction)
{
    direction.x = cos(yaw) * cos(pitch);
    direction.y = sin(yaw);
    direction.z = cos(yaw) * sin(pitch);
}

std::ostream& operator<<(std::ostream& os, const glm::mat4& mat) {
    for (int i = 0; i < 4; ++i) {
        os << "(";
        for (int j = 0; j < 4; ++j) {
            os << mat[i][j];
            if (j < 3) os << ", ";  // 每个元素后面加逗号
        }
        os << ")";
        if (i < 3) os << std::endl; // 行与行之间换行
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const glm::vec3& vec) {
    os << "(";
    for (int j = 0; j < 3; ++j) {
        os << vec[j];
        if (j < 2) os << ", ";  // 每个元素后面加逗号
    }
    os << ")";
    return os;
}