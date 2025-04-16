#ifndef MESHDISTANCEFIELD_HPP
#define MESHDISTANCEFIELD_HPP

#include "glm/glm.hpp"
#include <memory>

namespace MeshDistanceField {
	inline const float VoxelDensity = 0.2f;
	inline const int DistanceFieldResolutionScale = 8;
	//inline const int NumVoxelsPerLocalSpaceUnit = 7;
	inline const int NumMips = 1;
	inline const int MeshDistanceFieldObjectBorder = 1;
	inline const int UniqueDataBrickSize = 7;
	inline const int BandSizeInVoxels = 4;
}

class Mesh;

class MeshUtilities {
public:

	void GenerateMeshDistanceField(
		std::shared_ptr<Mesh> mesh
	);


private:

};

class SparseMeshDistanceFieldGenerateTask {

};


#endif // MESHDISTANCEFIELD_HPP

