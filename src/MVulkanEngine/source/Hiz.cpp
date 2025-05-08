#include "Hiz.hpp"

void Hiz::UpdateHizRes(glm::ivec2 basicResolution)
{
	glm::ivec2 res = basicResolution;

	int numLayers = 0;
	while (res.x >= 1 && res.y >= 1)
	{
		//hizRes[layer] = res;
		res /= 2;
		numLayers++;
	}

	hizRes.resize(numLayers);
	int layer = 0;
	res = basicResolution;
	while (res.x >= 1 && res.y >= 1)
	{
		hizRes[layer] = res;
		res /= 2;
		layer++;
	}
}
