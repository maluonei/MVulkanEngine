#ifndef MESHDISTANCEFIELD_HPP
#define MESHDISTANCEFIELD_HPP

#include "glm/glm.hpp"
#include <memory>
#include <vector>
#include <Util.hpp>
#include "Shaders/share/MeshDistanceField.h"

namespace MeshDistanceField {
	inline const float VoxelDensity = 0.2f;
	//inline const int DistanceFieldResolutionScale = 8;
	inline const int BrickSize = 8;
	//inline const int NumVoxelsPerLocalSpaceUnit = 7;
	inline const int NumMips = 1;
	inline const int MeshDistanceFieldObjectBorder = 1;
	inline const int UniqueDataBrickSize = 7;
	inline const int BandSizeInVoxels = 2;
}

struct Mesh;
class EmbreeScene;
class MVulkanTexture;

class SparseMeshDistanceField {
public:
	void GenerateMDFTexture();
	MeshDistanceFieldInput GetMDFBuffer();
public:
	float localToVolumnScale;
	BoundingBox volumeBounds;
	BoundingBox localSpaceMeshBounds;
	//std::vector<uint8_t> distanceFieldVolume;
	glm::ivec3 IndirectionDimensions;
	int32_t NumDistanceFieldBricks;
	glm::vec3 VolumeToVirtualUVScale;
	glm::vec3 VolumeToVirtualUVAdd;
	glm::vec2 DistanceFieldToVolumeScaleBias;
	std::shared_ptr<MVulkanTexture> mdfTexture;

	glm::ivec3 BrickDimensions;
	glm::ivec2 TextureResolution;
};


class MeshUtilities {
public:

	void GenerateMeshDistanceField(
		std::shared_ptr<Mesh> mesh,
		float distanceFieldResolutionScale,
		SparseMeshDistanceField& mdf
	);


private:

};

class SparseMeshDistanceFieldGenerateTask {
public:
	void GenerateMeshDistanceFieldInternal(
		std::shared_ptr<EmbreeScene> embreeScene,
		const std::vector<glm::vec3>* sampleDirections,
		float inLocalSpaceTraceDistance,
		BoundingBox inVolumeBounds,
		float inLocalToVolumeScale,
		glm::vec2 InDistanceFieldToVolumeScaleBias,
		glm::ivec3 inBrickCoordinate,
		glm::ivec3 inIndirectionSize
	);


public:
	std::vector<uint8_t> distanceFieldVolume;
	uint8_t brickMaxDistance;
	uint8_t brickMinDistance;
};

#endif // MESHDISTANCEFIELD_HPP

