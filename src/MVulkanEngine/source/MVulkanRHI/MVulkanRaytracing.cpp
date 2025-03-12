#include "MVulkanRHI/MVulkanRaytracing.hpp"
#include "Scene/Scene.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "spdlog/spdlog.h"
#include "MVulkanRHI/MVulkanCommand.hpp"

void TLASBuildInfo::Clean(VkDevice device) {
    instancetBuffer.Clean();
    asBuffer.Clean();
    scrathBuffer.Clean();

    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
    vkDestroyAccelerationStructureKHR =
        (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");


    vkDestroyAccelerationStructureKHR(device, tlas, nullptr);
}

void MVulkanRaytracing::Create(MVulkanDevice device)
{
    m_device = device;
}

void MVulkanRaytracing::Clean() {
    for (auto buffer : m_asBuffers) {
        buffer.Clean();
    }
    for (auto buffer : m_scrathBuffers) {
        buffer.Clean();
    }

    m_tlasBuildInfo.Clean(m_device.GetDevice());

    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
    vkDestroyAccelerationStructureKHR =
        (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkDestroyAccelerationStructureKHR");
    for (auto blas : m_blas) {
        vkDestroyAccelerationStructureKHR(m_device.GetDevice(), blas, nullptr);
    }
}


void MVulkanRaytracing::TestCreateAccelerationStructure(const std::shared_ptr<Scene>& scene)
{
    //auto names = scene->GetMeshNames();
    auto numPrims = scene->GetNumPrimInfos();

    m_buildInfos = std::vector<VkAccelerationStructureBuildGeometryInfoKHR>(numPrims);
    m_rangeInfos = std::vector<VkAccelerationStructureBuildRangeInfoKHR>(numPrims);
    m_blas = std::vector<VkAccelerationStructureKHR>(numPrims);
    m_asBuffers = std::vector<Buffer>(numPrims);
    m_scrathBuffers = std::vector<Buffer>(numPrims);

    std::vector<BLASBuildInfo>                                  blasBuildInfos(numPrims);

    auto rayTracingCommandList = Singleton<MVulkanEngine>::instance().GetRaytracingCommandList();

    //int i=0;
    for(auto index = 0; index < numPrims; index++)
    {
        auto mesh = scene->GetMesh(scene->m_primInfos[index].mesh_id);

        //spdlog::info("mesh->vertices.size():{0}", mesh->vertices.size());
        
        CreateBLAS(mesh, blasBuildInfos[index]);

        m_buildInfos[index] = blasBuildInfos[index].geometryInfo;
        m_rangeInfos[index] = blasBuildInfos[index].rangeInfo;
        m_blas[index] = blasBuildInfos[index].blas;
        m_asBuffers[index] = blasBuildInfos[index].asBuffer;
        m_scrathBuffers[index] = blasBuildInfos[index].scrathBuffer;

        //i++;
    }

    rayTracingCommandList.Reset();
    rayTracingCommandList.Begin();
    
    rayTracingCommandList.BuildAccelerationStructure(m_buildInfos, m_rangeInfos);

    //nvvk::accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
    //    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
    //VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    //barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    //barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    //vkCmdPipelineBarrier(rayTracingCommandList.GetBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    //    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    rayTracingCommandList.End();
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &rayTracingCommandList.GetBuffer();
    
    auto queue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);
    queue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    queue.WaitForQueueComplete();

    //build tlas
    //TLASBuildInfo tlasBuildInfo;
    CreateTLAS(m_asBuffers, m_tlasBuildInfo);
    //tlas = tlasBuildInfo.tlas;

    //std::vector<VkAccelerationStructureBuildGeometryInfoKHR> _tlasBuildInfos(1, m_tlasBuildInfo.geometryInfo);
    //std::vector<VkAccelerationStructureBuildRangeInfoKHR*> _tlasRangeInfos(1, m_tlasBuildInfo.rangeInfo);
    m_tlasBuildInfos = std::vector<VkAccelerationStructureBuildGeometryInfoKHR>(1, m_tlasBuildInfo.geometryInfo);
    m_tlasRangeInfos = std::vector<VkAccelerationStructureBuildRangeInfoKHR>(1, m_tlasBuildInfo.rangeInfo);
    


    {
        rayTracingCommandList.Reset();
        rayTracingCommandList.Begin();

        rayTracingCommandList.BuildAccelerationStructure(m_tlasBuildInfos, m_tlasRangeInfos);

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

    {
        blasBuildInfo.asBuffer = Buffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);
        BufferCreateInfo info0{};
        info0.size = accelerationStructureBufferSize;
        blasBuildInfo.asBuffer.Create(m_device, info0);
    }

    {
        blasBuildInfo.scrathBuffer = Buffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);
        BufferCreateInfo info1{};
        info1.size = buildScratchSize;
        blasBuildInfo.scrathBuffer.Create(m_device, info1);
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

    //blasBuildInfo.blas = new VkAccelerationStructureKHR();
    VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(m_device.GetDevice(), &asCreateInfo, nullptr, &blasBuildInfo.blas));

    //blasBuildInfo.rangeInfo = new VkAccelerationStructureBuildRangeInfoKHR();
    blasBuildInfo.rangeInfo.primitiveCount = primitiveCount; // 图元数量
    blasBuildInfo.rangeInfo.primitiveOffset = 0; // 图元数据偏移量
    blasBuildInfo.rangeInfo.firstVertex = 0; // 第一个顶点
    blasBuildInfo.rangeInfo.transformOffset = 0; // 变换偏移量

    blasBuildInfo.geometryInfo.dstAccelerationStructure = blasBuildInfo.blas;
    blasBuildInfo.geometryInfo.scratchData.deviceAddress = blasBuildInfo.scrathBuffer.GetBufferAddress(); // 临时缓冲区地址
}

void MVulkanRaytracing::CreateTLAS(std::vector<Buffer> asBuffers, TLASBuildInfo& tlasBuildInfo)
{
    tlasBuildInfo.asBuffer = Buffer(BufferType::ACCELERATION_STRUCTURE_STORAGE_BUFFER);
    
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR =
        (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkGetAccelerationStructureDeviceAddressKHR");

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
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = m_blas[i];
        vkGetAccelerationStructureDeviceAddressKHR(m_device.GetDevice(), &addressInfo);

        instances[i].accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(m_device.GetDevice(), &addressInfo);
    }

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

    tlasBuildInfo.instancesData = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    tlasBuildInfo.instancesData.data.deviceAddress = tlasBuildInfo.instancetBuffer.GetBufferAddress();
    tlasBuildInfo.geometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    tlasBuildInfo.geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasBuildInfo.geometry.geometry.instances = tlasBuildInfo.instancesData;

    bool update = false;
    tlasBuildInfo.geometryInfo = VkAccelerationStructureBuildGeometryInfoKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR);
    tlasBuildInfo.geometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    tlasBuildInfo.geometryInfo.geometryCount = 1;
    tlasBuildInfo.geometryInfo.pGeometries = &tlasBuildInfo.geometry;
    tlasBuildInfo.geometryInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlasBuildInfo.geometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildInfo.geometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    tlasBuildInfo.geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    tlasBuildInfo.geometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    tlasBuildInfo.geometryInfo.dstAccelerationStructure = VK_NULL_HANDLE;
    tlasBuildInfo.geometryInfo.ppGeometries = nullptr;
    tlasBuildInfo.geometryInfo.scratchData.deviceAddress = 0;

    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
    // 在设备创建后加载函数指针
    vkGetAccelerationStructureBuildSizesKHR =
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkGetAccelerationStructureBuildSizesKHR");
     
    uint32_t countInstance = instances.size();
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(m_device.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlasBuildInfo.geometryInfo,
        &countInstance, &sizeInfo);

    {
        tlasBuildInfo.asBuffer = Buffer(ACCELERATION_STRUCTURE_STORAGE_BUFFER);
        info.size = sizeInfo.accelerationStructureSize;
        tlasBuildInfo.asBuffer.Create(m_device, info);
    }

    {
        tlasBuildInfo.scrathBuffer = Buffer(ACCELERATION_STRUCTURE_STORAGE_BUFFER);
        info.size = sizeInfo.buildScratchSize;
        tlasBuildInfo.scrathBuffer.Create(m_device, info);
    }


    VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    createInfo.size = sizeInfo.accelerationStructureSize;
    createInfo.buffer = tlasBuildInfo.asBuffer.GetBuffer();
    createInfo.offset = 0;


    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
    // 在设备创建后加载函数指针
    vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_device.GetDevice(), "vkCreateAccelerationStructureKHR");

    VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(m_device.GetDevice(), &createInfo, nullptr, &tlasBuildInfo.tlas));

    tlasBuildInfo.geometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    tlasBuildInfo.geometryInfo.dstAccelerationStructure = tlasBuildInfo.tlas;
    tlasBuildInfo.geometryInfo.scratchData.deviceAddress = tlasBuildInfo.scrathBuffer.GetBufferAddress();

    tlasBuildInfo.rangeInfo.primitiveCount = countInstance;
    tlasBuildInfo.rangeInfo.firstVertex = 0;
    tlasBuildInfo.rangeInfo.primitiveOffset = 0;
    tlasBuildInfo.rangeInfo.transformOffset = 0;
}
