#ifndef DDGI_HPP
#define DDGI_HPP

#include <glm/glm.hpp>

//9.920306, 0.7, 3.9827316
//-11.015438, 6.619673, -4.404496
//            10.615406

//-11.015438, 0.7, -4.404496
//2.5, 1.3, 1.05

class DDGIVolume {
public:
	DDGIVolume(glm::vec3 startPosition, glm::vec3 offset);

	glm::vec3 GetProbePosition(int x, int y, int z);
	glm::vec3 GetProbePosition(int index);

private:
	glm::vec3 m_startPosition;
	glm::vec3 m_offset;
};

#endif