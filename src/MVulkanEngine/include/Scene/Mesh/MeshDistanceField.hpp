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

	//inline const glm::ivec3 MdfAtlasDims = { 1024, 1024, 8 };
	inline const glm::ivec3 MdfAtlasDims = { 1024, 1024, 8 };
	inline const float GMeshSDFSurfaceBiasExpand = 0.25f;
}

struct Mesh;
class EmbreeScene;
class MVulkanTexture;

class SparseMeshDistanceField {
public:
	//void GenerateMDFTexture();
	//MeshDistanceFieldInput GetMDFBuffer();
public:
	float localToVolumnScale;
	BoundingBox volumeBounds;
	BoundingBox localSpaceMeshBounds;
	BoundingBox worldSpaceMeshBounds;
	std::vector<uint8_t> distanceFieldVolume;
	std::vector<float> originDistanceFieldVolume;
	glm::ivec3 IndirectionDimensions;
	int32_t NumDistanceFieldBricks;
	glm::vec3 VolumeToVirtualUVScale;
	glm::vec3 VolumeToVirtualUVAdd;
	glm::vec2 DistanceFieldToVolumeScaleBias;
	int firstBrickIndex = 0;
	//std::shared_ptr<MVulkanTexture> mdfTexture;

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

class AsyncDistanceFieldTask {
public:
	AsyncDistanceFieldTask() = default;
	AsyncDistanceFieldTask(
		std::shared_ptr<Mesh> mesh,
		float distanceFieldResolutionScale
	);
	void DoWork();
public:
	std::shared_ptr<Mesh> m_mesh;
	float m_distanceFieldResolutionScale;
	SparseMeshDistanceField m_mdf;
};

class SparseMeshDistanceFieldGenerateTask {
public:
	SparseMeshDistanceFieldGenerateTask(std::shared_ptr<EmbreeScene> embreeScene,
		const std::vector<glm::vec3>* sampleDirections,
		float inLocalSpaceTraceDistance,
		BoundingBox inVolumeBounds,
		float inLocalToVolumeScale,
		glm::vec2 InDistanceFieldToVolumeScaleBias,
		glm::ivec3 inBrickCoordinate,
		glm::ivec3 inIndirectionSize);

	void GenerateMeshDistanceFieldInternal();
	//void GenerateMeshDistanceFieldInternal(
	//	std::shared_ptr<EmbreeScene> embreeScene,
	//	const std::vector<glm::vec3>* sampleDirections,
	//	float inLocalSpaceTraceDistance,
	//	BoundingBox inVolumeBounds,
	//	float inLocalToVolumeScale,
	//	glm::vec2 InDistanceFieldToVolumeScaleBias,
	//	glm::ivec3 inBrickCoordinate,
	//	glm::ivec3 inIndirectionSize
	//);


public:
	std::vector<uint8_t> distanceFieldVolume;
	std::vector<float> originDistanceFieldVolume;
	uint8_t brickMaxDistance;
	uint8_t brickMinDistance;

private:
	std::shared_ptr<EmbreeScene> embreeScene;
	const std::vector<glm::vec3>* sampleDirections;
	float localSpaceTraceDistance;
	BoundingBox volumeBounds;
	float localToVolumeScale;
	glm::vec2 distanceFieldToVolumeScaleBias;
	glm::ivec3 brickCoordinate;
	glm::ivec3 indirectionSize;
};

class MeshDistanceFieldAtlas {
public:
	void Init();
	int LoadData(
		SparseMeshDistanceField& field
	);

public:
	std::shared_ptr<MVulkanTexture> m_mdfAtlas = nullptr;
	std::shared_ptr<MVulkanTexture> m_mdfAtlas_origin = nullptr;
	std::vector<SparseMeshDistanceField> m_smdfs;
private:
	void loadBrickData(uint8_t* data);
	void loadBrickDataOrigin(float* data);

	int brickIndex = 0;
	int brickIndex_o = 0;
};

#endif // MESHDISTANCEFIELD_HPP

