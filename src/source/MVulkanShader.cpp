#include "MVulkanShader.h"

#

MVulkanShader::MVulkanShader()
{

}

MVulkanShader::MVulkanShader(std::string path, ShaderStageFlagBits stage)
{
	Init(path, stage);
}

void MVulkanShader::Init(std::string path, ShaderStageFlagBits stage)
{
	shader = Shader(path, stage);
}

void MVulkanShader::Create(VkDevice device)
{
	shader.Compile();

    shaderModule = createShaderModule(device, shader.GetCompiledShaderCode());
}

VkShaderModule MVulkanShader::createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void MVulkanShader::Clean(VkDevice device)
{
    vkDestroyShaderModule(device, shaderModule, nullptr);
}
