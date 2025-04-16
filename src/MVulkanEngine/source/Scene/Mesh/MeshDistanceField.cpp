#include "Scene/Mesh/MeshDistanceField.hpp"
#include <vector>
#include "Scene/Scene.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtx/component_wise.hpp>

void MeshUtilities::GenerateMeshDistanceField(std::shared_ptr<Mesh> mesh)
{
	const float numVoxelsPerLocalSpaceUnit = MeshDistanceField::VoxelDensity * MeshDistanceField::DistanceFieldResolutionScale;
	const int maxNumBlocksOneDim = 8;

	//todo: generate sample directions
	std::vector<glm::vec3> sampleDirections;
	{

	}

	BoundingBox localSpaceMeshBounds(mesh->m_box);
	{
		glm::vec3 center = localSpaceMeshBounds.GetCenter();
		glm::vec3 extent = glm::max(localSpaceMeshBounds.GetExtent(), glm::vec3(1.f));
		localSpaceMeshBounds.pMin = center - extent;
		localSpaceMeshBounds.pMax = center + extent;
	}

	//volumn scale:[-1, 1]
	const float localToVolumnScale = 1.f / glm::compMax(localSpaceMeshBounds.GetExtent());

	const glm::vec3 desiredDimensions = glm::vec3(localSpaceMeshBounds.GetSize() * numVoxelsPerLocalSpaceUnit / (float)MeshDistanceField::UniqueDataBrickSize);
	const glm::ivec3 mip0IndirectionDimensions = glm::ivec3(
		std::clamp((int)std::ceil(desiredDimensions.x), 1, maxNumBlocksOneDim),
		std::clamp((int)std::ceil(desiredDimensions.x), 1, maxNumBlocksOneDim),
		std::clamp((int)std::ceil(desiredDimensions.x), 1, maxNumBlocksOneDim)
	);

	for (int mipIndex = 0; mipIndex < MeshDistanceField::NumMips; mipIndex++) {
		const glm::ivec3 indirectionDimensions = glm::ivec3(
			(mip0IndirectionDimensions.x + (1 << mipIndex) - 1) / (1 << mipIndex),
			(mip0IndirectionDimensions.y + (1 << mipIndex) - 1) / (1 << mipIndex),
			(mip0IndirectionDimensions.z + (1 << mipIndex) - 1) / (1 << mipIndex)
		);

		const glm::vec3 texelObjectSpaceSize = localSpaceMeshBounds.GetSize() / glm::vec3(indirectionDimensions * MeshDistanceField::UniqueDataBrickSize - glm::ivec3(2 * MeshDistanceField::MeshDistanceFieldObjectBorder));
		const BoundingBox distanceFieldBounds = localSpaceMeshBounds.ExpandBy(texelObjectSpaceSize);

		const glm::vec3 indirectionVoxelSize = distanceFieldBounds.GetSize() / glm::vec3(indirectionDimensions);
		const float indirectionVoxelRadius = glm::length(indirectionVoxelSize);

		const glm::vec3 volumeSpaceDistanceFieldVoxelSize = indirectionVoxelSize * localToVolumnScale / glm::vec3(MeshDistanceField::UniqueDataBrickSize);
		const float maxDistanceForEncoding = glm::length(volumeSpaceDistanceFieldVoxelSize) * MeshDistanceField::BandSizeInVoxels;
		const float localSpaceTraceDistance = maxDistanceForEncoding / localToVolumnScale;
		const glm::vec2 distanceFieldToVolumeScaleBias(2.0f * maxDistanceForEncoding, -maxDistanceForEncoding);

		for (int z = 0; z < indirectionDimensions.z; z++){
			for (int y = 0; y < indirectionDimensions.y; y++){
				for (int x = 0; x < indirectionDimensions.x; x++) {

				}
			}
		}
	
	}

}
