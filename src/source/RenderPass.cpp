#include "RenderPass.hpp"
#include "MVulkanRHI/MVulkanShader.hpp"

RenderPass::RenderPass(MVulkanDevice device):
    m_device(device)
{
}


void RenderPass::Create()
{
}

void RenderPass::CreatePipeline(const std::string& vertPath, const std::string& fragPath)
{
    MVulkanShader vert(vertPath, ShaderStageFlagBits::VERTEX);
    MVulkanShader frag(fragPath, ShaderStageFlagBits::FRAGMENT);

    vert.Create(m_device.GetDevice());
    frag.Create(m_device.GetDevice());

    MVulkanShaderReflector vertReflector(vert.GetShader());
    MVulkanShaderReflector fragReflector(frag.GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();
    //fragReflector.GenerateDescriptorSet();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<VkDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<VkDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());

    //layouts.Create(m_device.GetDevice(), bindings);
    //descriptorSet.Create(m_device.GetDevice(), allocator.Get(), layouts.Get(), MAX_FRAMES_IN_FLIGHT);

    //glm::vec3 color0 = glm::vec3(1.f, 0.f, 0.f);
    //glm::vec3 color1 = glm::vec3(0.f, 1.f, 0.f);
    //phongShader.SetUBO0(glm::mat4(1.f), camera->GetViewMatrix(), camera->GetProjMatrix());
    //phongShader.SetUBO1(camera->GetPosition());

    //cbvs0.resize(MAX_FRAMES_IN_FLIGHT);
    //cbvs1.resize(MAX_FRAMES_IN_FLIGHT);
    //BufferCreateInfo _infoUBO0;
    //_infoUBO0.size = phongShader.GetBufferSizeBinding(0);
    //BufferCreateInfo _infoUBO1;
    //_infoUBO1.size = phongShader.GetBufferSizeBinding(1);

    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    cbvs0[i].Create(m_device, _infoUBO0);
    //    cbvs1[i].Create(m_device, _infoUBO1);
    //}

    //createTexture();
    //createSampler();

    //fs::path resourcePath = "F:/MVulkanEngine/resources/textures";
    //fs::path imagePath = resourcePath / "test.jpg";
    //image.Load(imagePath);
    //
    //ImageCreateInfo imageCreateInfo;
    //imageCreateInfo.width = image.Width();
    //imageCreateInfo.height = image.Height();
    //imageCreateInfo.format = image.Format();
    //imageCreateInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    //imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    //
    //testTexture.CreateAndLoadData(&transferList, device, imageCreateInfo, &image);
    //transferList.End();

    MVulkanDescriptorSetWrite write;

    //std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(MAX_FRAMES_IN_FLIGHT);
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    bufferInfos[i].resize(2);
    //
    //    bufferInfos[i][0].buffer = cbvs0[i].GetBuffer();
    //    bufferInfos[i][0].offset = 0;
    //    bufferInfos[i][0].range = phongShader.GetBufferSizeBinding(0);
    //
    //    bufferInfos[i][1].buffer = cbvs1[i].GetBuffer();
    //    bufferInfos[i][1].offset = 0;
    //    bufferInfos[i][1].range = phongShader.GetBufferSizeBinding(1);
    //}
    //
    //std::vector<std::vector<VkDescriptorImageInfo>> imageInfos(MAX_FRAMES_IN_FLIGHT);
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    imageInfos[i].resize(1);
    //
    //    imageInfos[i][0].sampler = sampler.GetSampler();
    //    imageInfos[i][0].imageView = testTexture.GetImageView();
    //    imageInfos[i][0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //}

    //write.Update(m_device.GetDevice(), descriptorSet.Get(), bufferInfos, imageInfos, MAX_FRAMES_IN_FLIGHT);
    //
    ////gbufferPipeline.Create(device, vert.GetShaderModule(), frag.GetShaderModule(), gbufferRenderPass.Get(), info, layouts.Get());
    //pipeline.Create(m_device, vert.GetShaderModule(), frag.GetShaderModule(), renderPass.Get(), info, layouts.Get(), 1);
    //
    //vert.Clean(m_device.GetDevice());
    //frag.Clean(m_device.GetDevice());
}

void RenderPass::CreateRenderPass()
{
}

void RenderPass::CreateFrameBuffer()
{
}
