#ifndef HIZ_HPP
#define HIZ_HPP

#include <glm/glm.hpp>
#include <vector>

struct Hiz{
public:
	void UpdateHizRes(glm::ivec2 basicResolution);

public:
	std::vector<glm::ivec2> hizRes;

};

#endif // !HIZ_HPP