#include "MVulkanShader.h"

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

MVulkanShaderReflector::MVulkanShaderReflector(Shader _shader)
    :shader(_shader), 
    spirvBinary(loadSPIRV(shader.GetCompiledShaderPath())),
    compiler(spirv_cross::CompilerGLSL(spirvBinary))
{
    resources = compiler.get_shader_resources();
}

//void MVulkanShaderReflector::loadShader(Shader _shader)
//{
//
//}

void MVulkanShaderReflector::GenerateVertexInputBindingDescription()
{
    uint32_t stride = 0;  // ���㲽��ֵ
    uint32_t binding = 0;  // ����󶨵�Ϊ 0

    for (const auto& input : resources.stage_inputs) {
        const spirv_cross::SPIRType& type = compiler.get_type(input.type_id);

        // ������ 32 λ��ȵĸ�����������ÿ�������Ĵ�С
        uint32_t size = 0;
        if (type.columns == 1) {
            switch (type.vecsize) {
            case 1: size = 4; break;  // float
            case 2: size = 8; break;  // vec2
            case 3: size = 12; break; // vec3
            case 4: size = 16; break; // vec4
            }
        }

        // ��ÿ����������Ĵ�С�ӵ��ܲ���ֵ��
        stride += size;
    }

    VkVertexInputBindingDescription binding_description = {};
    binding_description.binding = binding;          // �󶨵�����
    binding_description.stride = stride;            // ����ֵ��ÿ��������ֽڴ�С��
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // ÿ����

    // �������ֵ�Ͱ󶨵���Ϣ
    std::cout << "Binding: " << binding_description.binding
        << ", Stride: " << binding_description.stride << std::endl;

    binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;  // ÿʵ��
}

PipelineVertexInputStateInfo MVulkanShaderReflector::GenerateVertexInputAttributes()
{
    PipelineVertexInputStateInfo info;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    uint32_t location = 0;  // ���ڸ��������λ��

    // ������ɫ�����������
    for (const auto& input : resources.stage_inputs) {
        // ��ȡ������������Ϣ
        const spirv_cross::SPIRType& type = compiler.get_type(input.type_id);

        // ���� VkVertexInputAttributeDescription ����������
        VkVertexInputAttributeDescription attribute_description = {};
        attribute_description.location = compiler.get_decoration(input.id, spv::DecorationLocation);
        attribute_description.binding = 0;  // �󶨵㣬ͨ��Ϊ 0�������������������
        attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;  // Ĭ�ϼ����� float3 ����
        attribute_description.offset = 0;  // ������Ҫ���ݶ���ṹ�����

        // ���� SPIR-V �е�������Ϣ���ƶ� Vulkan �ĸ�ʽ
        if (type.columns == 1) {
            switch (type.width) {
            case 32:
                if (type.vecsize == 1) attribute_description.format = VK_FORMAT_R32_SFLOAT;
                else if (type.vecsize == 2) attribute_description.format = VK_FORMAT_R32G32_SFLOAT;
                else if (type.vecsize == 3) attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
                else if (type.vecsize == 4) attribute_description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
                // ���Ը�����Ҫ��չ֧������λ���� 16 λ��64 λ�ȣ�
            }
        }

        // �����ɵ� attribute_description ���뵽����
        attribute_descriptions.push_back(attribute_description);

        // ���� offset �� location ����Ӧ��һ���������
        location++;
    }

    attribute_descriptions[0].offset = offsetof(Vertex, position);
    if (attribute_descriptions.size() >= 2) attribute_descriptions[1].offset = offsetof(Vertex, normal);
    if (attribute_descriptions.size() >= 3) attribute_descriptions[2].offset = offsetof(Vertex, texcoord);

    // ʹ�õ����� attribute_descriptions
    for (const auto& attr : attribute_descriptions) {
        std::cout << "Location: " << attr.location
            << ", Format: " << attr.format
            << ", Offset: " << attr.offset << std::endl;
    }

    size_t bindingDescription_stride = 0;
    for (int i = 0; i < attribute_descriptions.size(); i++) {
        bindingDescription_stride += VertexSize[i];
    }

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    info.attribDesc = attribute_descriptions;
    info.bindingDesc = bindingDescription;

    return info;
}

void MVulkanShaderReflector::test(Shader shader)
{
    // ���� SPIR-V �ļ�
    std::vector<uint32_t> spirvBinary = loadSPIRV(shader.GetCompiledShaderPath());

    // ���� SPIRV-Cross �� GLSL ������
    spirv_cross::CompilerGLSL glslCompiler(spirvBinary);

    // ��ѡ������ GLSL ����ѡ��
    spirv_cross::CompilerGLSL::Options options;
    options.version = 460;  // ���� GLSL �汾
    options.es = false;     // ����� OpenGL ES����������Ϊ true
    glslCompiler.set_common_options(options);

    // �� SPIR-V ������Ϊ GLSL ����
    std::string glslCode = glslCompiler.compile();

    // ��� GLSL ����
    std::cout << glslCode << std::endl;
}
