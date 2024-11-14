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
