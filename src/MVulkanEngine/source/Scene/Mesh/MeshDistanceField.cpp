#include "Scene/Mesh/MeshDistanceField.hpp"
#include <vector>
#include "Scene/Scene.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "Managers/RandomGenerator.hpp"
#include "Scene/EmbreeScene.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "MultiThread/ParallelFor.hpp"

AsyncDistanceFieldTask::AsyncDistanceFieldTask(
	std::shared_ptr<Mesh> mesh,
	float distanceFieldResolutionScale
) :m_mesh(mesh), m_distanceFieldResolutionScale(distanceFieldResolutionScale) {};

void AsyncDistanceFieldTask::DoWork() {
	MeshUtilities util;
	util.GenerateMeshDistanceField(m_mesh, m_distanceFieldResolutionScale, m_mdf);
}


void MeshUtilities::GenerateMeshDistanceField(
	std::shared_ptr<Mesh> mesh,
	float distanceFieldResolutionScale,
	SparseMeshDistanceField& mdf)
{
	const float numVoxelsPerLocalSpaceUnit = MeshDistanceField::VoxelDensity * distanceFieldResolutionScale;
	const int maxNumBlocksOneDim = 8;

	//todo: generate sample directions
	const int numSamples = 256;
	std::vector<glm::vec3> sampleDirections(numSamples);
	{
		for (auto i = 0; i < numSamples; i++) {
			sampleDirections[i] = Singleton<RandomGenerator>::instance().GetRandomUnitVector();
		}
	}

	BoundingBox worldSpaceMeshBounds(mesh->m_box);
	BoundingBox localSpaceMeshBounds(mesh->m_box);
	{
		glm::vec3 center = localSpaceMeshBounds.GetCenter();
		glm::vec3 extent = glm::max(localSpaceMeshBounds.GetExtent(), glm::vec3(1.f));
		localSpaceMeshBounds.pMin = center - extent;
		localSpaceMeshBounds.pMax = center + extent;
	}

	//volumn scale:[-1, 1]
	//const float localToVolumnScale = 1.f / glm::compMax(localSpaceMeshBounds.GetExtent());

	const glm::vec3 desiredDimensions = glm::vec3(localSpaceMeshBounds.GetSize() * numVoxelsPerLocalSpaceUnit / (float)MeshDistanceField::UniqueDataBrickSize);
	const glm::ivec3 mip0IndirectionDimensions = glm::ivec3(
		std::clamp((int)std::ceil(desiredDimensions.x), 1, maxNumBlocksOneDim),
		std::clamp((int)std::ceil(desiredDimensions.x), 1, maxNumBlocksOneDim),
		std::clamp((int)std::ceil(desiredDimensions.x), 1, maxNumBlocksOneDim)
	);

	std::shared_ptr<EmbreeScene> embreeScene = std::make_shared<EmbreeScene>();
	embreeScene->Build(mesh);

	for (int mipIndex = 0; mipIndex < MeshDistanceField::NumMips; mipIndex++) {
		const glm::ivec3 indirectionDimensions = glm::ivec3(
			(mip0IndirectionDimensions.x + (1 << mipIndex) - 1) / (1 << mipIndex),
			(mip0IndirectionDimensions.y + (1 << mipIndex) - 1) / (1 << mipIndex),
			(mip0IndirectionDimensions.z + (1 << mipIndex) - 1) / (1 << mipIndex)
		);

		const glm::vec3 texelObjectSpaceSize = localSpaceMeshBounds.GetSize() / glm::vec3(indirectionDimensions * MeshDistanceField::UniqueDataBrickSize - glm::ivec3(2 * MeshDistanceField::MeshDistanceFieldObjectBorder));
		const BoundingBox distanceFieldBounds = localSpaceMeshBounds.ExpandBy(texelObjectSpaceSize + glm::vec3(1e-4f));

		const glm::vec3 indirectionVoxelSize = distanceFieldBounds.GetSize() / glm::vec3(indirectionDimensions);
		const float indirectionVoxelRadius = glm::length(indirectionVoxelSize);

		//const glm::vec3 volumeSpaceDistanceFieldVoxelSize = indirectionVoxelSize * localToVolumnScale / glm::vec3(MeshDistanceField::UniqueDataBrickSize);
		const glm::vec3 volumeSpaceDistanceFieldVoxelSize = indirectionVoxelSize / glm::vec3(MeshDistanceField::UniqueDataBrickSize);
		const float maxDistanceForEncoding = glm::length(volumeSpaceDistanceFieldVoxelSize) * MeshDistanceField::BandSizeInVoxels;
		//const float localSpaceTraceDistance = maxDistanceForEncoding / localToVolumnScale;
		const float localSpaceTraceDistance = maxDistanceForEncoding;
		const glm::vec2 distanceFieldToVolumeScaleBias(2.0f * maxDistanceForEncoding, -maxDistanceForEncoding);

		//SparseMeshDistanceField sparseMeshDistanceField;
		const uint32_t brickSizeBytes = MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * sizeof(uint8_t);
		uint32_t numBricks = indirectionDimensions.x * indirectionDimensions.y * indirectionDimensions.z;
		
		mdf.distanceFieldVolume.resize(brickSizeBytes * numBricks);
		mdf.originDistanceFieldVolume.resize(brickSizeBytes * numBricks);
		mdf.DistanceFieldToVolumeScaleBias = distanceFieldToVolumeScaleBias;
		mdf.IndirectionDimensions = indirectionDimensions;
		mdf.NumDistanceFieldBricks = numBricks;
		mdf.volumeBounds = distanceFieldBounds;
		mdf.localSpaceMeshBounds = localSpaceMeshBounds;
		mdf.BrickDimensions = indirectionDimensions;
		mdf.TextureResolution = { indirectionDimensions[0] * indirectionDimensions[1] * MeshDistanceField::BrickSize, indirectionDimensions[2] * MeshDistanceField::BrickSize };
		mdf.worldSpaceMeshBounds = worldSpaceMeshBounds;
		//mdf.localToVolumnScale = localToVolumnScale;
		mdf.localToVolumnScale = 1.f;
		//mdf.dimensions = 
		//mdf.GenerateMDFTexture();
		std::fill(mdf.distanceFieldVolume.begin(), mdf.distanceFieldVolume.end(), 0);
		std::fill(mdf.originDistanceFieldVolume.begin(), mdf.originDistanceFieldVolume.end(), 0);

		std::vector<SparseMeshDistanceFieldGenerateTask> tasks;

		for (int z = 0; z < indirectionDimensions.z; z++){
			for (int y = 0; y < indirectionDimensions.y; y++){
				for (int x = 0; x < indirectionDimensions.x; x++) {

					tasks.push_back(SparseMeshDistanceFieldGenerateTask(
						embreeScene,
						&sampleDirections,
						localSpaceTraceDistance,
						distanceFieldBounds,
						//localToVolumnScale,
						1.f,
						distanceFieldToVolumeScaleBias,
						glm::ivec3(x, y, z),
						indirectionDimensions
					));
				}
			}
		}

		ParallelFor(tasks.begin(), tasks.end(), [this](SparseMeshDistanceFieldGenerateTask& task)
			{
				task.GenerateMeshDistanceFieldInternal();
			},
			8
		);

		int index = 0;
		for (int z = 0; z < indirectionDimensions.z; z++) {
			for (int y = 0; y < indirectionDimensions.y; y++) {
				for (int x = 0; x < indirectionDimensions.x; x++) {
					uint32_t brickIndex = z + y * indirectionDimensions.z + x * indirectionDimensions.y * indirectionDimensions.z;
					memcpy(&mdf.distanceFieldVolume[brickIndex * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize], tasks[index].distanceFieldVolume.data(), sizeof(uint8_t) * tasks[index].distanceFieldVolume.size());
					memcpy(&mdf.originDistanceFieldVolume[brickIndex * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize], tasks[index].originDistanceFieldVolume.data(), sizeof(float) * tasks[index].originDistanceFieldVolume.size());
					index++;
				}
			}
		}
	
	}

	embreeScene->Clean();

}

SparseMeshDistanceFieldGenerateTask::SparseMeshDistanceFieldGenerateTask(
	std::shared_ptr<EmbreeScene> embreeScene,
	const std::vector<glm::vec3>* sampleDirections,
	float inLocalSpaceTraceDistance,
	BoundingBox inVolumeBounds,
	float inLocalToVolumeScale,
	glm::vec2 InDistanceFieldToVolumeScaleBias,
	glm::ivec3 inBrickCoordinate,
	glm::ivec3 inIndirectionSize):
	embreeScene(embreeScene),
	sampleDirections(sampleDirections),
	localSpaceTraceDistance(inLocalSpaceTraceDistance),
	volumeBounds(inVolumeBounds),
	localToVolumeScale(inLocalToVolumeScale),
	distanceFieldToVolumeScaleBias(InDistanceFieldToVolumeScaleBias),
	brickCoordinate(inBrickCoordinate),
	indirectionSize(inIndirectionSize){ }

void SparseMeshDistanceFieldGenerateTask::GenerateMeshDistanceFieldInternal()
{
	uint8_t brickMaxDistance;
	uint8_t brickMinDistance;

	const glm::vec3 indirectionVoxelSize = volumeBounds.GetSize() / glm::vec3(indirectionSize);
	const glm::vec3 distanceFieldVoxelSize = indirectionVoxelSize / glm::vec3(MeshDistanceField::UniqueDataBrickSize);
	//const glm::vec3 distanceFieldVoxelSize = indirectionVoxelSize / glm::vec3(6.f);
	const glm::vec3 brickMinPosition = volumeBounds.pMin + glm::vec3(brickCoordinate) * indirectionVoxelSize;// -distanceFieldVoxelSize;

	distanceFieldVolume.resize(MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize, 0);
	originDistanceFieldVolume.resize(MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize, 0);

	for (auto zIndex = 0; zIndex < MeshDistanceField::BrickSize; zIndex++) {
		for (auto yIndex = 0; yIndex < MeshDistanceField::BrickSize; yIndex++) {
			for (auto xIndex = 0; xIndex < MeshDistanceField::BrickSize; xIndex++) {
				
				const glm::vec3 randomOffset = Singleton<RandomGenerator>::instance().GetRandomUnitVector() * 1e-4f * localSpaceTraceDistance;
				const glm::vec3 voxelPosition = glm::vec3(xIndex, yIndex, zIndex) * distanceFieldVoxelSize + brickMinPosition;// +randomOffset;
				const int index = (zIndex * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize) + (yIndex * MeshDistanceField::BrickSize) + xIndex;
				
				int hitNum = 0;
				int hitBackfaceNum = 0;

				float minLocalSpaceDistance = localSpaceTraceDistance;

				for (auto i = 0; i < sampleDirections->size(); i++) {
					auto direction = (*sampleDirections)[i];

					auto hit = embreeScene->RayCast(voxelPosition, direction);
					if (hit.hit) {
						hitNum++;
						hitBackfaceNum += (hit.outSide ? 0 : 1);

						float distance = hit.distance * localSpaceTraceDistance;
						
						if (distance < minLocalSpaceDistance) {
							minLocalSpaceDistance = distance;
						}
					}
				}

				if (hitNum > 0 && hitBackfaceNum > 0.25f * sampleDirections->size()) {
					minLocalSpaceDistance *= -1;
				}

				const float volumeSpaceDistance = minLocalSpaceDistance;// *localToVolumeScale;
				const float rescaledDistance = (volumeSpaceDistance - distanceFieldToVolumeScaleBias.y) / distanceFieldToVolumeScaleBias.x;
				const uint8_t quantizedDistance = std::clamp(uint8_t(rescaledDistance * 255.0f + .5f), (uint8_t)0, (uint8_t)255);
				//const uint8_t quantizedOriginDistance = std::clamp(uint8_t(volumeSpaceDistance * 255.0f + .5f), (uint8_t)0, (uint8_t)255);
				const float quantizedOriginDistance = volumeSpaceDistance / 3.f;
				//float clampedVolumeSpaceDistance = std::clamp(volumeSpaceDistance, -0.5f / 255.f, 1.f - 0.5f / 255.f);
				//const uint8_t quantizedDistance = std::clamp(uint8_t(clampedVolumeSpaceDistance * 255.0f + .5f), (uint8_t)0, (uint8_t)255);

				distanceFieldVolume[index] = quantizedDistance;
				originDistanceFieldVolume[index] = quantizedOriginDistance;
				brickMinDistance = std::min(brickMinDistance, quantizedDistance);
				brickMaxDistance = std::max(brickMaxDistance, quantizedDistance);
			}
		}
	}
}


//void SparseMeshDistanceField::GenerateMDFTexture()
//{
//	{
//		//std::vector<float> floatDatas(distanceFieldVolume.size());
//		//for (auto i = 0; i < distanceFieldVolume.size(); i++) {
//		//	floatDatas[i] = static_cast<float>(distanceFieldVolume[i]) / 255.f;
//		//	//floatDatas[i] = (float)i / distanceFieldVolume.size();
//		//}
//
//		mdfTexture = std::make_shared<MVulkanTexture>();
//
//		ImageCreateInfo imageInfo;
//		ImageViewCreateInfo viewInfo;
//		imageInfo.arrayLength = 1;
//		imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//		imageInfo.width = IndirectionDimensions[0] * MeshDistanceField::BrickSize;
//		imageInfo.height = IndirectionDimensions[1] * MeshDistanceField::BrickSize;
//		imageInfo.depth = IndirectionDimensions[2] * MeshDistanceField::BrickSize;
//		imageInfo.format = VK_FORMAT_R8_UNORM;
//		imageInfo.type = VK_IMAGE_TYPE_3D;
//
//		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
//		viewInfo.format = imageInfo.format;
//		viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
//		viewInfo.baseMipLevel = 0;
//		viewInfo.levelCount = 1;
//		viewInfo.baseArrayLayer = 0;
//		viewInfo.layerCount = 1;
//
//		Singleton<MVulkanEngine>::instance().CreateImage(mdfTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
//
//		//Singleton<MVulkanEngine>::instance().LoadTextureData(mdfTexture, distanceFieldVolume.data(), distanceFieldVolume.size() * sizeof(uint8_t), 0);
//	}
//}

//MeshDistanceFieldInput SparseMeshDistanceField::GetMDFBuffer() {
//	MeshDistanceFieldInput input;
//
//	input.distanceFieldToVolumeScaleBias = DistanceFieldToVolumeScaleBias;
//	input.volumeCenter = volumeBounds.GetCenter();
//	input.volumeOffset = volumeBounds.GetExtent();
//	input.volumeToWorldScale = localSpaceMeshBounds.GetExtent();
//
//	//glm::mat4 m(1.0f);
//	glm::mat4 trans = glm::translate(glm::mat4(1.f), worldSpaceMeshBounds.GetCenter());
//	glm::mat4 scale = glm::scale(glm::mat4(1.0f), worldSpaceMeshBounds.GetExtent());
//	input.VolumeToWorld = trans * scale;
//	input.WorldToVolume = glm::inverse(input.VolumeToWorld);
//
//	input.dimensions = BrickDimensions;
//	input.mdfTextureResolusion = TextureResolution;
//
//	return input;
//
//	//input.numDistanceFieldBricks = NumDistanceFieldBricks;
//	//input.mdfTexture = mdfTexture->GetTexture();
//}


void MeshDistanceFieldAtlas::Init() {
	m_mdfAtlas = std::make_shared<MVulkanTexture>();
	m_mdfAtlas_origin = std::make_shared<MVulkanTexture>();

	ImageCreateInfo imageInfo;
	ImageViewCreateInfo viewInfo;
	imageInfo.arrayLength = 1;
	imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.width = MeshDistanceField::MdfAtlasDims.x;
	imageInfo.height = MeshDistanceField::MdfAtlasDims.y;
	imageInfo.depth = MeshDistanceField::MdfAtlasDims.z;
	imageInfo.format = VK_FORMAT_R8_UNORM;
	imageInfo.type = VK_IMAGE_TYPE_3D;

	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	viewInfo.format = imageInfo.format;
	viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.baseMipLevel = 0;
	viewInfo.levelCount = 1;
	viewInfo.baseArrayLayer = 0;
	viewInfo.layerCount = 1;

	Singleton<MVulkanEngine>::instance().CreateImage(m_mdfAtlas, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);

	imageInfo.format = VK_FORMAT_R32_SFLOAT;
	viewInfo.format = imageInfo.format;
	Singleton<MVulkanEngine>::instance().CreateImage(m_mdfAtlas_origin, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
}

int MeshDistanceFieldAtlas::LoadData(
	SparseMeshDistanceField& field
) {
	int numBricks = field.BrickDimensions.x * field.BrickDimensions.y * field.BrickDimensions.z;
	int firstBrick = brickIndex;

	for (auto x = 0; x < field.BrickDimensions.x; x++) {
		for (auto y = 0; y < field.BrickDimensions.y; y++) {
			for (auto z = 0; z < field.BrickDimensions.z; z++) {
				int localBrickIndex = (x * field.BrickDimensions.y + y) * field.BrickDimensions.z + z;
				int brickVoxelNum = MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize;

				loadBrickData(&(field.distanceFieldVolume[localBrickIndex * brickVoxelNum]));
				loadBrickDataOrigin(&(field.originDistanceFieldVolume[localBrickIndex * brickVoxelNum]));
			}
		}
	}

	field.firstBrickIndex = firstBrick;
	m_smdfs.push_back(field);
	return firstBrick;
}

void MeshDistanceFieldAtlas::loadBrickData(
	uint8_t* data
) {
	//int numBricks = field.BrickDimensions.x * field.BrickDimensions.y * field.BrickDimensions.z;
	int d = MeshDistanceField::MdfAtlasDims.x / MeshDistanceField::BrickSize;

	int originX = brickIndex / d;
	int originY = (brickIndex % d);
	int originZ = 0;
	//int originZ = brickIndex % 32;

	VkOffset3D origin = { originX * MeshDistanceField::BrickSize, originY * MeshDistanceField::BrickSize, originZ * MeshDistanceField::BrickSize };
	VkExtent3D scale = { MeshDistanceField::BrickSize, MeshDistanceField::BrickSize, MeshDistanceField::BrickSize };

	Singleton<MVulkanEngine>::instance().LoadTextureData(
		m_mdfAtlas, data, sizeof(uint8_t) * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize,
		0, origin, scale);

	brickIndex++;

	const int maxBrickIndex = 128 * 128;
	if (brickIndex > maxBrickIndex) {
		spdlog::error("too many mdf bricks");
	}
}

void MeshDistanceFieldAtlas::loadBrickDataOrigin(float* data)
{
	//int numBricks = field.BrickDimensions.x * field.BrickDimensions.y * field.BrickDimensions.z;
	int d = MeshDistanceField::MdfAtlasDims.x / MeshDistanceField::BrickSize;

	int originX = brickIndex_o / d;
	int originY = (brickIndex_o % d);
	int originZ = 0;
	//int originZ = brickIndex % 32;

	VkOffset3D origin = { originX * MeshDistanceField::BrickSize, originY * MeshDistanceField::BrickSize, originZ * MeshDistanceField::BrickSize };
	VkExtent3D scale = { MeshDistanceField::BrickSize, MeshDistanceField::BrickSize, MeshDistanceField::BrickSize };

	Singleton<MVulkanEngine>::instance().LoadTextureData(
		m_mdfAtlas_origin, data, sizeof(float) * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize * MeshDistanceField::BrickSize,
		0, origin, scale);

	brickIndex_o++;
}
