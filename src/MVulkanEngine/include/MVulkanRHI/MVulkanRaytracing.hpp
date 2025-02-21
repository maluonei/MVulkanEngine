#pragma once
#ifndef MVULKANRAYTRACING_HPP
#define MVULKANRAYTRACING_HPP

#include <vulkan/vulkan_core.h>
#include <vector>
#include "Scene/Scene.hpp"
#include "MVulkanRHI/MVulkanDevice.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

//struct AccelKHR
//{
//	VkAccelerationStructureKHR m_accel = VK_NULL_HANDLE;
//	MVulkanBuffer              m_buffer;
//	VkDeviceAddress            m_address{0};
//};
//
//// Single Geometry information, multiple can be used in a single BLAS
//struct AccelerationStructureGeometryInfo
//{
//	VkAccelerationStructureGeometryKHR       geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
//	VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
//};
//
//struct AccelerationStructureBuildData
//{
//  VkAccelerationStructureTypeKHR asType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;  // Mandatory to set
//
//  // Collection of geometries for the acceleration structure.
//  std::vector<VkAccelerationStructureGeometryKHR> asGeometry;
//  // Build range information corresponding to each geometry.
//  std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildRangeInfo;
//  // Build information required for acceleration structure.
//  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
//  // Size information for acceleration structure build resources.
//  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
//
//  //// Adds a geometry with its build range information to the acceleration structure.
//  //void addGeometry(const VkAccelerationStructureGeometryKHR& asGeom, const VkAccelerationStructureBuildRangeInfoKHR& offset);
//  //void addGeometry(const AccelerationStructureGeometryInfo& asGeom);
////
//  //AccelerationStructureGeometryInfo makeInstanceGeometry(size_t numInstances, VkDeviceAddress instanceBufferAddr);
////
//  //// Configures the build information and calculates the necessary size information.
//  VkAccelerationStructureBuildSizesInfoKHR finalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags);
////
//  //// Creates an acceleration structure based on the current build and size info.
//  VkAccelerationStructureCreateInfoKHR makeCreateInfo() const;
////
//  //// Commands to build the acceleration structure in a Vulkan command buffer.
//  //void cmdBuildAccelerationStructure(VkCommandBuffer cmd, VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress scratchAddress);
////
//  //// Commands to update the acceleration structure in a Vulkan command buffer.
//  //void cmdUpdateAccelerationStructure(VkCommandBuffer cmd, VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress scratchAddress);
////
//  //// Checks if the compact flag is set for the build.
//  //bool hasCompactFlag() const { return buildInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR; }
//};
//
//class BlasBuilder
//{
//public:
//	BlasBuilder(VkDevice device, MRaytracingCommandList list);
//
//	void buildAccelerationStructures(std::vector<AccelerationStructureBuildData>& blasBuildData,
//															std::vector<AccelKHR>&                 blasAccel,
//															const std::vector<VkDeviceAddress>&          scratchAddress,
//															VkDeviceSize                                 hintMaxBudget,
//															VkDeviceSize                                 currentBudget,
//															uint32_t& currentQueryIdx);
//
//	VkDeviceSize getScratchSize(VkDeviceSize                                       hintMaxBudget,
//								const std::vector<AccelerationStructureBuildData>& buildData,
//								uint32_t                                           minAlignment = 128) const;
//
//	void getScratchAddresses(VkDeviceSize                                             hintMaxBudget,
//							 const std::vector<AccelerationStructureBuildData>& buildData,
//							 VkDeviceAddress                                          scratchBufferAddress,
//							 std::vector<VkDeviceAddress>&                            scratchAddresses,
//							 uint32_t                                                 minAlignment = 128);
//private:
//	MRaytracingCommandList	m_commandList;
//	VkDevice				m_device;
//	uint32_t                m_currentBlasIdx{0};
//	uint32_t                m_currentQueryIdx{0};
//};
//
//
//class MVulkanRayTracingBuilder
//{
//public:
//	struct BlasInput
//	{
//		// Data used to build acceleration structure geometry
//		std::vector<VkAccelerationStructureGeometryKHR>       	m_asGeometry;
//		std::vector<VkAccelerationStructureBuildRangeInfoKHR> 	m_asBuildOffsetInfo;
//		VkBuildAccelerationStructureFlagsKHR                  	m_flags{0};
//	};
//
//	void Create(MVulkanDevice device);
//
//	void buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags);
//	bool hasFlag(VkFlags item, VkFlags flag) { return (item & flag) == flag; }
//public:
//	MVulkanDevice												m_device;
//	std::vector<AccelKHR> 										m_blas;  // Bottom-level acceleration structure
//	AccelKHR              										m_tlas;  // Top-level acceleration structure
//	MRaytracingCommandList										m_list;
//};
//
////class Scene;
//class MVulkanRaytracingGeometry {
//public:
//
//	
//public:
//	static constexpr uint32_t	m_indicesPerPrimitive = 3; // Only triangle meshes are supported
//
//	void Create();
//	uint64_t GetAccelerationStructureAddress();
//
//private:
//	auto objectToVkGeometryKHR(const std::shared_ptr<Mesh>& mesh);
//	void createBottomLevelAS(const std::shared_ptr<Scene>& scene);
//	
//
//
//private:
//	MVulkanDevice				m_device;
//	VkAccelerationStructureKHR	m_handle = VK_NULL_HANDLE;
//	VkDeviceAddress				m_address = 0;
//
//	uint64_t					m_accelerationStructureCompactedSize = 0;
//	MVulkanRayTracingBuilder	m_rtBuilder;
//};
//
//
//

struct BLASBuildInfo {
    VkAccelerationStructureGeometryKHR          geometry;
    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo;
    VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
    VkAccelerationStructureKHR blas;
    Buffer asBuffer;
    Buffer scrathBuffer;

    //void Clean();
};

struct TLASBuildInfo {
    VkAccelerationStructureGeometryKHR          geometry;
    VkAccelerationStructureGeometryInstancesDataKHR instancesData;
    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo;
    VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
    VkAccelerationStructureKHR tlas;
    Buffer instancetBuffer;
    Buffer asBuffer;
    Buffer scrathBuffer;

    void Clean(VkDevice device);
};

class MVulkanRaytracing
{
public:


public:
    void Create(MVulkanDevice device);
    
    void TestCreateAccelerationStructure(const std::shared_ptr<Scene>& scene);
    
    VkAccelerationStructureKHR GetTLAS() {
        return m_tlasBuildInfo.tlas;
    }

    void Clean();

private:
    void CreateBLAS(const std::shared_ptr<Mesh>& mesh, BLASBuildInfo& blasBuildInfo);
    void CreateTLAS(std::vector<Buffer> asBuffers, TLASBuildInfo& tlasBuildInfo);

    std::vector<VkAccelerationStructureBuildGeometryInfoKHR>    m_buildInfos;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>       m_rangeInfos;
    std::vector<VkAccelerationStructureKHR>                     m_blas;
    std::vector<Buffer>                                         m_asBuffers;
    std::vector<Buffer>                                         m_scrathBuffers;

    std::vector<VkAccelerationStructureBuildGeometryInfoKHR>    m_tlasBuildInfos;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>       m_tlasRangeInfos;
    TLASBuildInfo                                               m_tlasBuildInfo;

    //Buffer                                                      m_tlasBuffer;
    //Buffer                                                      m_tlasScrathBuffer;

    MVulkanDevice m_device;
};


#endif