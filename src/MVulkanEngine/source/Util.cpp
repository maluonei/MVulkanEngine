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

glm::vec4 RotationMatrixToQuaternion(glm::mat3 R)
{
    glm::vec4 q;
    float trace = R[0][0] + R[1][1] + R[2][2];

    if (trace > 0) {
        float S = sqrt(trace + 1.0f) * 2; // S = 4 * w
        q.w = 0.25f * S;
        q.x = (R[2][1] - R[1][2]) / S;
        q.y = (R[0][2] - R[2][0]) / S;
        q.z = (R[1][0] - R[0][1]) / S;
    }
    else {
        if (R[0][0] > R[1][1] && R[0][0] > R[2][2]) {
            float S = sqrt(1.0f + R[0][0] - R[1][1] - R[2][2]) * 2;
            q.w = (R[2][1] - R[1][2]) / S;
            q.x = 0.25f * S;
            q.y = (R[0][1] + R[1][0]) / S;
            q.z = (R[0][2] + R[2][0]) / S;
        }
        else if (R[1][1] > R[2][2]) {
            float S = sqrt(1.0f + R[1][1] - R[0][0] - R[2][2]) * 2;
            q.w = (R[0][2] - R[2][0]) / S;
            q.x = (R[0][1] + R[1][0]) / S;
            q.y = 0.25f * S;
            q.z = (R[1][2] + R[2][1]) / S;
        }
        else {
            float S = sqrt(1.0f + R[2][2] - R[0][0] - R[1][1]) * 2;
            q.w = (R[1][0] - R[0][1]) / S;
            q.x = (R[0][2] + R[2][0]) / S;
            q.y = (R[1][2] + R[2][1]) / S;
            q.z = 0.25f * S;
        }
    }
    return q;
}

BoundingBox::BoundingBox(glm::vec3 inPmin, glm::vec3 inPmax):
    pMin(inPmin), pMax(inPmax)
{
}

const glm::vec3 BoundingBox::GetCenter() const
{
    return (pMin + pMax) / 2.f;
}

const glm::vec3 BoundingBox::GetExtent() const
{
    return (pMax - pMin) / 2.f;
}

const glm::vec3 BoundingBox::GetSize() const
{
    return pMax - pMin;
}

const BoundingBox BoundingBox::ExpandBy(const glm::vec3 scale) const
{
    return BoundingBox(pMin-scale, pMax+scale);
}
