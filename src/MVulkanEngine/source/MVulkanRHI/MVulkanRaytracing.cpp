#include "MVulkanRHI/MVulkanRaytracing.hpp"
#include "Scene/Scene.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "spdlog/spdlog.h"
#include "MVulkanRHI/MVulkanCommand.hpp"


//void MVulkanRayTracingBuilder::Create(MVulkanDevice device)
//{
//  m_device = device;
//}
//
//void MVulkanRayTracingBuilder::buildBlas(const std::vector<BlasInput>& input,
//                                         VkBuildAccelerationStructureFlagsKHR flags)
//{
//    //m_cmdPool.init(m_device, m_queueIndex);
//    auto         numBlas = static_cast<uint32_t>(input.size());
//    VkDeviceSize asTotalSize{0};     // Memory size of all allocated BLAS
//    VkDeviceSize maxScratchSize{0};  // Largest scratch size
//    
//    std::vector<AccelerationStructureBuildData> blasBuildData(numBlas);
//    m_blas.resize(numBlas);  // Resize to hold all the BLAS
//    for(uint32_t idx = 0; idx < numBlas; idx++)
//    {
//      blasBuildData[idx].asType           = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
//      blasBuildData[idx].asGeometry       = input[idx].m_asGeometry;
//      blasBuildData[idx].asBuildRangeInfo = input[idx].m_asBuildOffsetInfo;
//    
//      auto sizeInfo  = blasBuildData[idx].finalizeGeometry(m_device.GetDevice(), input[idx].m_flags | flags);
//      maxScratchSize = std::max(maxScratchSize, sizeInfo.buildScratchSize);
//    }
//    
//    VkDeviceSize hintMaxBudget{256'000'000};  // 256 MB
//    
//    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
//    Buffer blasScratchBuffer(BufferType::STORAGE_BUFFER);
//    
//    bool hasCompaction = hasFlag(flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
//    
//    BlasBuilder blasBuilder(m_device.GetDevice());
//    
//    uint32_t minAlignment = 128; /*m_rtASProperties.minAccelerationStructureScratchOffsetAlignment*/
//    // 1) finding the largest scratch size
//    VkDeviceSize scratchSize = blasBuilder.getScratchSize(hintMaxBudget, blasBuildData, minAlignment);
//    // 2) allocating the scratch buffer
//    BufferCreateInfo info{};
//    info.size = scratchSize;
//    blasScratchBuffer.Create(m_device, info);
//    //blasScratchBuffer = 
//    //    m_alloc->createBuffer(scratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
//    // 3) getting the device address for the scratch buffer
//    std::vector<VkDeviceAddress> scratchAddresses;
//    blasBuilder.getScratchAddresses(hintMaxBudget, blasBuildData, blasScratchBuffer.GetBufferAddress(), scratchAddresses, minAlignment);
//  
//    bool finished = false;
//    do
//    {
//      {
//        VkCommandBuffer cmd = m_cmdPool.createCommandBuffer();
//        finished = blasBuilder.cmdCreateParallelBlas(cmd, blasBuildData, m_blas, scratchAddresses, hintMaxBudget);
//        m_cmdPool.submitAndWait(cmd);
//      }
//      if(hasCompaction)
//      {
//        VkCommandBuffer cmd = m_cmdPool.createCommandBuffer();
//        blasBuilder.cmdCompactBlas(cmd, blasBuildData, m_blas);
//        m_cmdPool.submitAndWait(cmd);  // Submit command buffer and call vkQueueWaitIdle
//        blasBuilder.destroyNonCompactedBlas();
//      }
//    } while(!finished);
//    //LOGI("%s\n", blasBuilder.getStatistics().toString().c_str());
//    
//    // Clean up
//    //m_alloc->finalizeAndReleaseStaging();
//    //m_alloc->destroy(blasScratchBuffer);
//    //m_cmdPool.deinit();
//}
//
//void BlasBuilder::buildAccelerationStructures(std::vector<AccelerationStructureBuildData>& blasBuildData,
//    std::vector<AccelKHR>& blasAccel, const std::vector<VkDeviceAddress>& scratchAddress, VkDeviceSize hintMaxBudget,
//    VkDeviceSize currentBudget, uint32_t& currentQueryIdx)
//{
//    // Temporary vectors for storing build-related data
//    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> collectedBuildInfo;
//    std::vector<VkAccelerationStructureKHR>                  collectedAccel;
//    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>   collectedRangeInfo;
//
//    // Pre-allocate memory based on the number of BLAS to be built
//    collectedBuildInfo.reserve(blasBuildData.size());
//    collectedAccel.reserve(blasBuildData.size());
//    collectedRangeInfo.reserve(blasBuildData.size());
//
//    // Initialize the total budget used in this function call
//    VkDeviceSize budgetUsed = 0;
//
//    // Loop through BLAS data while there is scratch address space and budget available
//    while(collectedBuildInfo.size() < scratchAddress.size() && currentBudget + budgetUsed < hintMaxBudget
//          && m_currentBlasIdx < blasBuildData.size())
//    {
//        auto&                                data       = blasBuildData[m_currentBlasIdx];
//        VkAccelerationStructureCreateInfoKHR createInfo = data.makeCreateInfo();
//
//        // Create and store acceleration structure
//        blasAccel[m_currentBlasIdx] = m_alloc->createAcceleration(createInfo);
//        collectedAccel.push_back(blasAccel[m_currentBlasIdx].accel);
//
//        // Setup build information for the current BLAS
//        data.buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
//        data.buildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
//        data.buildInfo.dstAccelerationStructure  = blasAccel[m_currentBlasIdx].accel;
//        data.buildInfo.scratchData.deviceAddress = scratchAddress[m_currentBlasIdx % scratchAddress.size()];
//        data.buildInfo.pGeometries               = data.asGeometry.data();
//        collectedBuildInfo.push_back(data.buildInfo);
//        collectedRangeInfo.push_back(data.asBuildRangeInfo.data());
//
//        // Update the used budget with the size of the current structure
//        budgetUsed += data.sizeInfo.accelerationStructureSize;
//        m_currentBlasIdx++;
//  }
//
//  // Command to build the acceleration structures on the GPU
//  vkCmdBuildAccelerationStructuresKHR(cmd, static_cast<uint32_t>(collectedBuildInfo.size()), collectedBuildInfo.data(),
//                                      collectedRangeInfo.data());
//
//  // Barrier to ensure proper synchronization after building
//  accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
//
//  // If a query pool is available, record the properties of the built acceleration structures
//  if(m_queryPool)
//  {
//    vkCmdWriteAccelerationStructuresPropertiesKHR(cmd, static_cast<uint32_t>(collectedAccel.size()), collectedAccel.data(),
//                                                  VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, m_queryPool,
//                                                  currentQueryIdx);
//    currentQueryIdx += static_cast<uint32_t>(collectedAccel.size());
//  }
//
//  // Return the total budget used in this operation
//  return budgetUsed;
//}
//
//// Find if the total scratch size is within the budget, otherwise return n-time the max scratch size that fits in the budget
//VkDeviceSize BlasBuilder::getScratchSize(VkDeviceSize                                             hintMaxBudget,
//                                               const std::vector<AccelerationStructureBuildData>& buildData,
//                                               uint32_t minAlignment /*= 128*/) const
//{
//  ScratchSizeInfo sizeInfo     = calculateScratchAlignedSizes(buildData, minAlignment);
//  VkDeviceSize    maxScratch   = sizeInfo.maxScratch;
//  VkDeviceSize    totalScratch = sizeInfo.totalScratch;
//
//  if(totalScratch < hintMaxBudget)
//  {
//    return totalScratch;
//  }
//  else
//  {
//    uint64_t numScratch = std::max(uint64_t(1), hintMaxBudget / maxScratch);
//    numScratch          = std::min(numScratch, buildData.size());
//    return numScratch * maxScratch;
//  }
//}
//
//// Return the scratch addresses fitting the scrath strategy (see above)
//void BlasBuilder::getScratchAddresses(VkDeviceSize                                       hintMaxBudget,
//                                      const std::vector<AccelerationStructureBuildData>& buildData,
//                                      VkDeviceAddress               scratchBufferAddress,
//                                      std::vector<VkDeviceAddress>& scratchAddresses,
//                                      uint32_t                      minAlignment /*=128*/)
//{
//  ScratchSizeInfo sizeInfo     = calculateScratchAlignedSizes(buildData, minAlignment);
//  VkDeviceSize    maxScratch   = sizeInfo.maxScratch;
//  VkDeviceSize    totalScratch = sizeInfo.totalScratch;
//
//  // Strategy 1: scratch was large enough for all BLAS, return the addresses in order
//  if(totalScratch < hintMaxBudget)
//  {
//    VkDeviceAddress address = {};
//    for(auto& buildInfo : buildData)
//    {
//      scratchAddresses.push_back(scratchBufferAddress + address);
//      VkDeviceSize alignedSize = nvh::align_up(buildInfo.sizeInfo.buildScratchSize, minAlignment);
//      address += alignedSize;
//    }
//  }
//  // Strategy 2: there are n-times the max scratch fitting in the budget
//  else
//  {
//    // Make sure there is at least one scratch buffer, and not more than the number of BLAS
//    uint64_t numScratch = std::max(uint64_t(1), hintMaxBudget / maxScratch);
//    numScratch          = std::min(numScratch, buildData.size());
//
//    VkDeviceAddress address = {};
//    for(int i = 0; i < numScratch; i++)
//    {
//      scratchAddresses.push_back(scratchBufferAddress + address);
//      address += maxScratch;
//    }
//  }
//}
//
//
//
//void MVulkanRaytracingGeometry::Create()
//{
//    //static auto vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device.GetDevice(), "vkCreateAccelerationStructureKHR"));
////
//    //vkCreateAccelerationStructureKHR(
//    //    m_device.GetDevice(),
//    //    
//    //);
//
//    
//}
//
//uint64_t MVulkanRaytracingGeometry::GetAccelerationStructureAddress()
//{
//
//}
//
//auto MVulkanRaytracingGeometry::objectToVkGeometryKHR(const std::shared_ptr<Mesh>& mesh)
//{
//    // BLAS builder requires raw device addresses.
//    VkDeviceAddress vertexAddress = mesh->m_vertexBuffer->GetBufferAddress();
//    VkDeviceAddress indexAddress  = mesh->m_indexBuffer->GetBufferAddress();
//
//    uint32_t maxPrimitiveCount = mesh->indices.size() / 3;
//
//    // Describe buffer as array of VertexObj.
//    VkAccelerationStructureGeometryTrianglesDataKHR triangles{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
//    triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
//    triangles.vertexData.deviceAddress = vertexAddress;
//    triangles.vertexStride             = sizeof(Vertex);
//    // Describe index data (32-bit unsigned int)
//    triangles.indexType               = VK_INDEX_TYPE_UINT32;
//    triangles.indexData.deviceAddress = indexAddress;
//    // Indicate identity transform by setting transformData to null device pointer.
//    //triangles.transformData = {};
//    triangles.maxVertex = mesh->vertices.size() - 1;
//
//    // Identify the above data as containing opaque triangles.
//    VkAccelerationStructureGeometryKHR asGeom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
//    asGeom.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
//    asGeom.flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
//    asGeom.geometry.triangles = triangles;
//
//    // The entire array will be used to build the BLAS.
//    VkAccelerationStructureBuildRangeInfoKHR offset;
//    offset.firstVertex     = 0;
//    offset.primitiveCount  = maxPrimitiveCount;
//    offset.primitiveOffset = 0;
//    offset.transformOffset = 0;
//
//    // Our blas is made from only one geometry, but could be made of many geometries
//    MVulkanRayTracingBuilder::BlasInput input;
//    input.m_asGeometry.emplace_back(asGeom);
//    input.m_asBuildOffsetInfo.emplace_back(offset);
//
//    return input;
//}
//
//void MVulkanRaytracingGeometry::createBottomLevelAS(const std::shared_ptr<Scene>& scene)
//{
//    // BLAS - Storing each primitive in a geometry
//    std::vector<MVulkanRayTracingBuilder::BlasInput> allBlas;
//    allBlas.reserve(1);
//
//    auto names = scene->GetMeshNames();
//    for(auto name:names)
//    {
//        auto mesh = scene->GetMesh(name);
//        auto blas = objectToVkGeometryKHR(mesh);
//
//        // We could add more geometry in each BLAS, but we add only one for now
//        allBlas.emplace_back(blas);
//    }
//  
//    m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
//}
//
//


//MVRT::MVRT()
//{
//    vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkCreateAccelerationStructureKHR");
//    
//}

void MVulkanRaytracing::Create(MVulkanDevice device)
{
    m_device = device;
}


void MVulkanRaytracing::TestCreateAccelerationStructure(const std::shared_ptr<Scene>& scene)
{
    auto names = scene->GetMeshNames();

    //build blas
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR>    buildInfos(names.size());
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>      rangeInfos(names.size());
    std::vector<VkAccelerationStructureKHR*>                    blas(names.size());
    std::vector<Buffer>                                         asBuffers(names.size());
    std::vector<Buffer>                                         scrathBuffers(names.size());

    std::vector<BLASBuildInfo>                                  blasBuildInfos(names.size());

    auto rayTracingCommandList = Singleton<MVulkanEngine>::instance().GetRaytracingCommandList();

    int i=0;
    for(auto name:names)
    {
        auto mesh = scene->GetMesh(name);

        spdlog::info("mesh->vertices.size():{0}", mesh->vertices.size());
        
        CreateBLAS(mesh, blasBuildInfos[i]);

        buildInfos[i] = blasBuildInfos[i].geometryInfo;
        rangeInfos[i] = blasBuildInfos[i].rangeInfo;
        blas[i] = blasBuildInfos[i].blas;
        asBuffers[i] = blasBuildInfos[i].asBuffer;
        scrathBuffers[i] = blasBuildInfos[i].scrathBuffer;

        i++;
    }

    rayTracingCommandList.Reset();
    rayTracingCommandList.Begin();
    
    rayTracingCommandList.BuildAccelerationStructure(buildInfos, rangeInfos);
    
    rayTracingCommandList.End();
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &rayTracingCommandList.GetBuffer();
    
    auto queue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);
    queue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    queue.WaitForQueueComplete();

    //build tlas
    //uint32_t countInstance = asBuffers.size();
    TLASBuildInfo tlasBuildInfo;
    CreateTLAS(asBuffers, tlasBuildInfo);

    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> tlasBuildInfos(1, tlasBuildInfo.geometryInfo);
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> tlasRangeInfos(1, tlasBuildInfo.rangeInfo);
    {
        rayTracingCommandList.Reset();
        rayTracingCommandList.Begin();

        rayTracingCommandList.BuildAccelerationStructure(tlasBuildInfos, tlasRangeInfos);

        rayTracingCommandList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &rayTracingCommandList.GetBuffer();

        auto queue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);
        queue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        queue.WaitForQueueComplete();
    }
}

void MVulkanRaytracing::CreateBLAS(const std::shared_ptr<Mesh>& mesh, BLASBuildInfo& blasBuildInfo)
{
    //BLASBuildInfo blasBuildInfo;

    //VkAccelerationStructureGeometryKHR geometry = {};
    blasBuildInfo.geometry = {};
    blasBuildInfo.geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blasBuildInfo.geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    blasBuildInfo.geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    // 配置顶点数据
    blasBuildInfo.geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    blasBuildInfo.geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT; // 顶点格式
    blasBuildInfo.geometry.geometry.triangles.vertexData.deviceAddress = mesh->m_vertexBuffer->GetBufferAddress(); // 顶点缓冲区地址
    blasBuildInfo.geometry.geometry.triangles.vertexStride = sizeof(Vertex); // 顶点步幅
    blasBuildInfo.geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32; // 索引类型
    blasBuildInfo.geometry.geometry.triangles.indexData.deviceAddress = mesh->m_indexBuffer->GetBufferAddress(); // 索引缓冲区地址
    blasBuildInfo.geometry.geometry.triangles.maxVertex = mesh->vertices.size(); // 顶点数量

    uint32_t primitiveCount = mesh->indices.size() / 3;

    //VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    blasBuildInfo.geometryInfo = {};
    blasBuildInfo.geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blasBuildInfo.geometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blasBuildInfo.geometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blasBuildInfo.geometryInfo.geometryCount = 1;
    blasBuildInfo.geometryInfo.pGeometries = &blasBuildInfo.geometry;

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
    vkGetAccelerationStructureBuildSizesKHR =
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkGetAccelerationStructureBuildSizesKHR");

    vkGetAccelerationStructureBuildSizesKHR(
        m_device.GetDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &blasBuildInfo.geometryInfo,
        &primitiveCount,
        &buildSizesInfo
    );

    VkDeviceSize accelerationStructureBufferSize = buildSizesInfo.accelerationStructureSize;
    VkDeviceSize buildScratchSize = buildSizesInfo.buildScratchSize;
    VkDeviceSize updateScratchSize = buildSizesInfo.updateScratchSize;

    //Buffer asBuffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);
    {
        blasBuildInfo.asBuffer = Buffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);
        BufferCreateInfo info{};
        info.size = accelerationStructureBufferSize;
        blasBuildInfo.asBuffer.Create(m_device, info);
    }
    //
    //Buffer scrathBuffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);
    {
        blasBuildInfo.scrathBuffer = Buffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);
        BufferCreateInfo info{};
        info.size = buildScratchSize;
        blasBuildInfo.scrathBuffer.Create(m_device, info);
    }

    VkAccelerationStructureCreateInfoKHR asCreateInfo = {};
    asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    asCreateInfo.buffer = blasBuildInfo.asBuffer.GetBuffer();
    asCreateInfo.offset = 0;
    asCreateInfo.size = buildSizesInfo.accelerationStructureSize;
    asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
    // 在设备创建后加载函数指针
    vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkCreateAccelerationStructureKHR");

    //VkAccelerationStructureKHR accelerationStructure;
    blasBuildInfo.blas = new VkAccelerationStructureKHR();
    vkCreateAccelerationStructureKHR(m_device.GetDevice(), &asCreateInfo, nullptr, blasBuildInfo.blas);

    blasBuildInfo.rangeInfo = new VkAccelerationStructureBuildRangeInfoKHR();
    blasBuildInfo.rangeInfo->primitiveCount = primitiveCount; // 图元数量
    blasBuildInfo.rangeInfo->primitiveOffset = 0; // 图元数据偏移量
    blasBuildInfo.rangeInfo->firstVertex = 0; // 第一个顶点
    blasBuildInfo.rangeInfo->transformOffset = 0; // 变换偏移量

    blasBuildInfo.geometryInfo.dstAccelerationStructure = *blasBuildInfo.blas;
    blasBuildInfo.geometryInfo.scratchData.deviceAddress = blasBuildInfo.scrathBuffer.GetBufferAddress(); // 临时缓冲区地址

    //return blasBuildInfo;
}

void MVulkanRaytracing::CreateTLAS(std::vector<Buffer> asBuffers, TLASBuildInfo& tlasBuildInfo)
{
    //TLASBuildInfo tlasBuildInfo;

    tlasBuildInfo.asBuffer = Buffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);

    std::vector<VkAccelerationStructureInstanceKHR> instances(asBuffers.size());
    for (auto i = 0; i < asBuffers.size(); i++) {
        instances[i].transform = {
            1.0f, 0.0f, 0.0f, 0.0f,  // 第一行
            0.0f, 1.0f, 0.0f, 0.0f,  // 第二行
            0.0f, 0.0f, 1.0f, 0.0f   // 第三行
        };

        instances[i].instanceCustomIndex = i;
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = 0;
        instances[i].flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
        instances[i].accelerationStructureReference = asBuffers[i].GetBufferAddress();
    }

    //Buffer instanceBuffer(BufferType::ACCELERATION_STRUCTURE_BUILD_INPUT_BUFFER);
    tlasBuildInfo.instancetBuffer = Buffer(BufferType::ACCELERATION_STRUCTURE_BUILD_INPUT_BUFFER);
    BufferCreateInfo info;
    info.size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

    {
        auto rayTracingCommandList = Singleton<MVulkanEngine>::instance().GetRaytracingCommandList();

        rayTracingCommandList.Reset();
        rayTracingCommandList.Begin();

        tlasBuildInfo.instancetBuffer.CreateAndLoadData(&rayTracingCommandList, m_device, info, instances.data());

        rayTracingCommandList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &rayTracingCommandList.GetBuffer();

        auto queue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);
        queue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        queue.WaitForQueueComplete();
    }

    //VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    tlasBuildInfo.instancesData = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    tlasBuildInfo.instancesData.data.deviceAddress = tlasBuildInfo.instancetBuffer.GetBufferAddress();
    //VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    tlasBuildInfo.geometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    tlasBuildInfo.geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasBuildInfo.geometry.geometry.instances = tlasBuildInfo.instancesData;

    bool update = false;
    //VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    tlasBuildInfo.geometryInfo = VkAccelerationStructureBuildGeometryInfoKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR);
    tlasBuildInfo.geometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    tlasBuildInfo.geometryInfo.geometryCount = 1;
    tlasBuildInfo.geometryInfo.pGeometries = &tlasBuildInfo.geometry;
    tlasBuildInfo.geometryInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlasBuildInfo.geometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildInfo.geometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
    // 在设备创建后加载函数指针
    vkGetAccelerationStructureBuildSizesKHR =
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkGetAccelerationStructureBuildSizesKHR");

    uint32_t countInstance = 1;
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(m_device.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlasBuildInfo.geometryInfo,
        &countInstance, &sizeInfo);

    info.size = sizeInfo.accelerationStructureSize;
    tlasBuildInfo.asBuffer.Create(m_device, info);

    VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    createInfo.size = sizeInfo.accelerationStructureSize;
    createInfo.buffer = tlasBuildInfo.asBuffer.GetBuffer();
    createInfo.offset = 0;

    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
    // 在设备创建后加载函数指针
    vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkCreateAccelerationStructureKHR");

    //VkAccelerationStructureKHR accelerationStructure;
    vkCreateAccelerationStructureKHR(m_device.GetDevice(), &createInfo, nullptr, tlasBuildInfo.tlas);

    //Buffer scratchBuffer(STORAGE_BUFFER);
    tlasBuildInfo.scrathBuffer = Buffer(STORAGE_BUFFER);
    info.size = sizeInfo.buildScratchSize;
    tlasBuildInfo.scrathBuffer.Create(m_device, info);

    //VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, tlasBuildInfo.scrathBuffer.GetBuffer() };
    //VkDeviceAddress           scratchAddress = vkGetBufferDeviceAddress(m_device.GetDevice(), &bufferInfo);

    tlasBuildInfo.geometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    tlasBuildInfo.geometryInfo.dstAccelerationStructure = *tlasBuildInfo.tlas;
    tlasBuildInfo.geometryInfo.scratchData.deviceAddress = tlasBuildInfo.scrathBuffer.GetBufferAddress();

    tlasBuildInfo.rangeInfo = new VkAccelerationStructureBuildRangeInfoKHR();
    tlasBuildInfo.rangeInfo->primitiveCount = countInstance;
    tlasBuildInfo.rangeInfo->firstVertex = 0;
    tlasBuildInfo.rangeInfo->primitiveOffset = 0;
    tlasBuildInfo.rangeInfo->transformOffset = 0;

    //return tlasBuildInfo;
}
