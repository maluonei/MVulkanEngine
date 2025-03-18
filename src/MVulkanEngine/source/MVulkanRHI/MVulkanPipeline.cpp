#include "MVulkanRHI/MVulkanPipeline.hpp"
#include <vector>
#include <stdexcept>

MVulkanPipeline::MVulkanPipeline()
{
}

void MVulkanPipeline::Create()
{
}

void MVulkanPipeline::Clean()
{
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
}

void MVulkanGraphicsPipeline::Create(
    VkDevice device, 
    MVulkanPilelineCreateInfo info, 
    VkShaderModule vertShaderModule, 
    VkShaderModule fragShaderModule, 
    VkRenderPass renderPass,
    PipelineVertexInputStateInfo vertexStateInfo, 
    VkDescriptorSetLayout layout, 
    uint32_t numAttachments,
    std::string vertEntryPoint,
    std::string fragEntryPoint)
{
    m_device = device;
    m_info = info;

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = vertEntryPoint.c_str();

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = fragEntryPoint.c_str();

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pVertexBindingDescriptions = &vertexStateInfo.bindingDesc;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = vertexStateInfo.attribDesc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexStateInfo.attribDesc.size());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = info.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = info.lineWidth;
    //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode = info.cullmode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    //rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    //colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    //colorBlendAttachment.blendEnable = VK_FALSE;
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(numAttachments);
    for(auto i=0;i<numAttachments;i++)
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachments[i] = colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = numAttachments;
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = info.depthTestEnable;
    depthStencil.depthWriteEnable = info.depthWriteEnable;
    //depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthCompareOp = info.depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &layout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));

}

void MVulkanRaytracingPipeline::Create(VkDevice device)
{
    //enum StageIndices
    //{
    //    eRaygen,
    //    eMiss,
    //    eClosestHit,
    //    eShaderGroupCount
    //};
//
    //std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
    //VkPipelineShaderStageCreateInfo              stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    //stage.pName = "main";  // All the same entry point
    //// Raygen
    //stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rgen.spv", true, defaultSearchPaths, true));
    //stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    //stages[eRaygen] = stage;
    //// Miss
    //stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rmiss.spv", true, defaultSearchPaths, true));
    //stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    //stages[eMiss] = stage;
    //// Hit Group - Closest Hit
    //stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rchit.spv", true, defaultSearchPaths, true));
    //stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    //stages[eClosestHit] = stage;
//
    //std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;
//
    //VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {};
    //pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    //pipelineCreateInfo.stageCount = 3;
    //pipelineCreateInfo.pStages = stages;  // ��ɫ���׶�����
    //pipelineCreateInfo.groupCount = m_rtShaderGroups.size();
    //pipelineCreateInfo.pGroups = m_rtShaderGroups.data(); // ��ɫ��������
    //pipelineCreateInfo.maxRecursionDepth = 3;
    //pipelineCreateInfo.layout = pipelineLayout;
//
    //// ��������
    //VkPipeline rayTracingPipeline;
    //VkResult result = vkCreateRaytracingPipeline(device, VK_NULL_HANDLE, &pipelineCreateInfo, nullptr, &m_pipeline);
    //if (result != VK_SUCCESS) {
    //    // �������
    //}
}

void MVulkanComputePipeline::Create(
    VkDevice device, 
    VkShaderModule compShaderModule, 
    VkDescriptorSetLayout layout,
    std::string entryPoint)
{
    m_device = device;

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = compShaderModule;
    computeShaderStageInfo.pName = entryPoint.c_str();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &layout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.stage = computeShaderStageInfo;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    //vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void MVulkanGraphicsPipeline::Create(VkDevice device, MVulkanPilelineCreateInfo info, VkShaderModule vertShaderModule, VkShaderModule geomShaderModule, VkShaderModule fragShaderModule, VkRenderPass renderPass, PipelineVertexInputStateInfo vertexStateInfo, VkDescriptorSetLayout layout, uint32_t numAttachments, std::string vertEntryPoint, std::string geomEntryPoint, std::string fragEntryPoint)
{
    m_device = device;
    m_info = info;

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = vertEntryPoint.c_str();

    VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
    geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    geomShaderStageInfo.module = geomShaderModule;
    geomShaderStageInfo.pName = geomEntryPoint.c_str();

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = fragEntryPoint.c_str();

    VkPipelineShaderStageCreateInfo shaderStages[] = { 
        vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pVertexBindingDescriptions = &vertexStateInfo.bindingDesc;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = vertexStateInfo.attribDesc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexStateInfo.attribDesc.size());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode = info.cullmode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    //rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    //colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    //colorBlendAttachment.blendEnable = VK_FALSE;
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(numAttachments);
    for (auto i = 0; i < numAttachments; i++)
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachments[i] = colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = numAttachments;
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = info.depthTestEnable;
    depthStencil.depthWriteEnable = info.depthWriteEnable;
    //depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthCompareOp = info.depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &layout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 3;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));

}
