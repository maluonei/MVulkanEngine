#include "MVulkanRHI/MVulkanShader.hpp"
#include "Scene/Scene.hpp"
#include <spdlog/spdlog.h>

MVulkanShader::MVulkanShader(std::string entryPoint, bool compileEveryTime)
    :m_compileEveryTime(compileEveryTime), m_entryName(entryPoint)
{

}

MVulkanShader::MVulkanShader(std::string path, ShaderStageFlagBits stage, std::string entryPoint, bool compileEveryTime)
    :m_compileEveryTime(compileEveryTime), m_entryName(entryPoint)
{
	Init(path, stage);
}

void MVulkanShader::Init(std::string path, ShaderStageFlagBits stage)
{
	m_shader = Shader(path, stage, m_entryName, m_compileEveryTime);
}

void MVulkanShader::Create(VkDevice device)
{
    m_device = device;
    m_shader.Compile();

    m_shaderModule = createShaderModule(m_shader.GetCompiledShaderCode());
}

VkShaderModule MVulkanShader::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void MVulkanShader::Clean()
{
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
}

MVulkanShaderReflector::MVulkanShaderReflector(Shader _shader)
    :m_shader(_shader), 
    m_spirvBinary(loadSPIRV(m_shader.GetCompiledShaderPath())),
    m_compiler(spirv_cross::CompilerGLSL(m_spirvBinary))
{
    m_resources = m_compiler.get_shader_resources();
}

void MVulkanShaderReflector::GenerateVertexInputBindingDescription()
{
    uint32_t stride = 0;  // 顶点步进值
    uint32_t binding = 0;  // 假设绑定点为 0

    for (const auto& input : m_resources.stage_inputs) {
        const spirv_cross::SPIRType& type = m_compiler.get_type(input.type_id);

        // 假设是 32 位宽度的浮点数，计算每个变量的大小
        uint32_t size = 0;
        if (type.columns == 1) {
            switch (type.vecsize) {
            case 1: size = 4; break;  // float
            case 2: size = 8; break;  // vec2
            case 3: size = 12; break; // vec3
            case 4: size = 16; break; // vec4
            }
        }

        // 将每个输入变量的大小加到总步进值中
        stride += size;
    }

    VkVertexInputBindingDescription binding_description = {};
    binding_description.binding = binding;          // 绑定点索引
    binding_description.stride = stride;            // 步进值（每个顶点的字节大小）
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // 每顶点

    // 输出步进值和绑定点信息
    spdlog::info("Binding:{0}, Stride:{1}", binding_description.binding, binding_description.stride);

    binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;  // 每实例
}

bool compVkVertexInputAttributeDescription(const VkVertexInputAttributeDescription& d0, const VkVertexInputAttributeDescription& d1) {
    return d0.location < d1.location;
}

PipelineVertexInputStateInfo MVulkanShaderReflector::GenerateVertexInputAttributes()
{
    PipelineVertexInputStateInfo info;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    uint32_t location = 0;  // 用于跟踪输入的位置

    // 遍历着色器的输入变量
    for (const auto& input : m_resources.stage_inputs) {
        // 获取变量的类型信息
        const spirv_cross::SPIRType& type = m_compiler.get_type(input.type_id);

        // 创建 VkVertexInputAttributeDescription 并设置属性
        VkVertexInputAttributeDescription attribute_description = {};
        attribute_description.location = m_compiler.get_decoration(input.id, spv::DecorationLocation);
        //attribute_description.location = location;
        attribute_description.binding = 0;  // 绑定点，通常为 0，具体根据你的需求调整
        attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;  // 默认假设是 float3 类型
        attribute_description.offset = 0;  // 这里需要根据顶点结构体计算

        // 根据 SPIR-V 中的类型信息，推断 Vulkan 的格式
        if (type.columns == 1) {
            switch (type.width) {
            case 32:
                if (type.vecsize == 1) attribute_description.format = VK_FORMAT_R32_SFLOAT;
                else if (type.vecsize == 2) attribute_description.format = VK_FORMAT_R32G32_SFLOAT;
                else if (type.vecsize == 3) attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
                else if (type.vecsize == 4) attribute_description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
                // 可以根据需要扩展支持其他位宽（如 16 位、64 位等）
            }
        }

        // 将生成的 attribute_description 加入到数组
        attribute_descriptions.push_back(attribute_description);

        // 更新 offset 和 location 以适应下一个输入变量
        location++;
    }

    std::sort(attribute_descriptions.begin(), attribute_descriptions.end(), compVkVertexInputAttributeDescription);

    if (attribute_descriptions.size() >= 1) attribute_descriptions[0].offset = offsetof(Vertex, position);
    if (attribute_descriptions.size() >= 2) attribute_descriptions[1].offset = offsetof(Vertex, normal);
    if (attribute_descriptions.size() >= 3) attribute_descriptions[2].offset = offsetof(Vertex, texcoord);
    if (attribute_descriptions.size() >= 4) attribute_descriptions[3].offset = offsetof(Vertex, tangent);
    if (attribute_descriptions.size() >= 5) attribute_descriptions[4].offset = offsetof(Vertex, bitangent);

    // 使用的最终 attribute_descriptions
    for (const auto& attr : attribute_descriptions) {
        //std::cout << "Location: " << attr.location
        //    << ", Format: " << attr.format
        //    << ", Offset: " << attr.offset << std::endl;
        spdlog::info("Location:{0}, Offset:{1}", attr.location, attr.offset);
    }

    size_t bindingDescription_stride = 0;
    for (int i = 0; i < attribute_descriptions.size(); i++) {
        bindingDescription_stride += VertexSize[i];
    }

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = 14 * sizeof(float);

    //if (attribute_descriptions.size() >= 1) bindingDescription.stride += 3 * sizeof(float);
    //if (attribute_descriptions.size() >= 2) bindingDescription.stride += 3 * sizeof(float);
    //if (attribute_descriptions.size() >= 3) bindingDescription.stride += 2 * sizeof(float);
    //if (attribute_descriptions.size() >= 4) bindingDescription.stride += 3 * sizeof(float);
    //if (attribute_descriptions.size() >= 5) bindingDescription.stride += 3 * sizeof(float);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    info.attribDesc = attribute_descriptions;
    info.bindingDesc = bindingDescription;

    return info;
}

MVulkanDescriptorSet MVulkanShaderReflector::GenerateDescriptorSet()
{
    for (const auto& ub : m_resources.uniform_buffers) {
        auto& type = m_compiler.get_type(ub.type_id);
        // 获取 Descriptor Set 和绑定点
        uint32_t set = m_compiler.get_decoration(ub.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(ub.id, spv::DecorationBinding);
        size_t size = m_compiler.get_declared_struct_size(type);
        spdlog::info("Uniform Buffer:{0}, Set:{1}, Binding:{2}, size:{3}", ub.name, set, binding, size);
    }

    // 处理 sampled images (纹理)
    for (const auto& image : m_resources.sampled_images) {
        uint32_t set = m_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(image.id, spv::DecorationBinding);
        spdlog::info("Sampled Image:{0}, Set:{1}, Binding:{2}", image.name, set, binding);
    }

    return MVulkanDescriptorSet();
}

ShaderReflectorOut MVulkanShaderReflector::GenerateShaderReflactorOut()
{
    ShaderStageFlagBits stage;
    std::string entryPointName;

    ShaderReflectorOut out;
    auto shader_stage = m_compiler.get_entry_points_and_stages();
    for (const auto& _stage : shader_stage) {
        switch (_stage.execution_model) {
        case spv::ExecutionModel::ExecutionModelVertex:
            stage = ShaderStageFlagBits::VERTEX;
            break;
        case spv::ExecutionModelFragment:
            stage = ShaderStageFlagBits::FRAGMENT;
            break;
        case spv::ExecutionModelGLCompute:
            stage = ShaderStageFlagBits::COMPUTE;
            break;
        case spv::ExecutionModelGeometry:
            stage = ShaderStageFlagBits::GEOMETRY;
            break;
        case spv::ExecutionModelTaskEXT:
            stage = ShaderStageFlagBits::TASK;
            break;
        case spv::ExecutionModelMeshEXT:
            stage = ShaderStageFlagBits::MESH;
            break;
        default:
            stage = ShaderStageFlagBits::VERTEX;
            break;
        }
        entryPointName = _stage.name;
    }


    for (const auto& ub : m_resources.uniform_buffers) {
        auto& type = m_compiler.get_type(ub.type_id);
        // 获取 Descriptor Set 和绑定点
        uint32_t set = m_compiler.get_decoration(ub.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(ub.id, spv::DecorationBinding);
        size_t size = m_compiler.get_declared_struct_size(type);
       
        spdlog::info("Uniform Buffer:{0}, Set:{1}, Binding:{2}, size:{3}", ub.name, set, binding, size);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "UBO " << ub.name << " has array size: " << arrayLength << std::endl;
        }

        out.uniformBuffers.push_back(ShaderResourceInfo{ ub.name, stage, set, binding, size, 0, arrayLength });
    }

    for (const auto& sb : m_resources.storage_buffers) {
        //if (sb.name.find("counter.var") != std::string::npos) {
        //    continue;
        //

        auto& type = m_compiler.get_type(sb.type_id);
        // 获取 Descriptor Set 和绑定点
        uint32_t set = m_compiler.get_decoration(sb.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(sb.id, spv::DecorationBinding);
        size_t size = m_compiler.get_declared_struct_size(type);

        spdlog::info("Storage Buffer:{0}, Set:{1}, Binding:{2}, size:{3}", sb.name, set, binding, size);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "UBO " << ub.name << " has array size: " << arrayLength << std::endl;
        }

        out.storageBuffers.push_back(ShaderResourceInfo{ sb.name, stage, set, binding, size, 0, arrayLength });
    }

    // 处理 sampled images (纹理) -- glsl
    for (const auto& image : m_resources.sampled_images) {
        auto& type = m_compiler.get_type(image.type_id);
        uint32_t set = m_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(image.id, spv::DecorationBinding);

        spdlog::info("Sampled Image:{0}, Set:{1}, Binding:{2}", image.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Image " << image.name << " has array size: " << arrayLength << std::endl;
        }

        out.combinedImageSamplers.push_back(ShaderResourceInfo{ image.name, stage, set, binding, 0, 0, arrayLength });
    }

    // 处理 seperate images (纹理) -- hlsl
    for (const auto& image : m_resources.separate_images) {
        auto& type = m_compiler.get_type(image.type_id);
        uint32_t set = m_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(image.id, spv::DecorationBinding);

        spdlog::info("Separate Image:{0}, Set:{1}, Binding:{2}", image.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Image " << image.name << " has array size: " << arrayLength << std::endl;
        }

        out.seperateImages.push_back(ShaderResourceInfo{ image.name, stage, set, binding, 0, 0, arrayLength });
    }

    //storage Images
    for (const auto& image : m_resources.storage_images) {
        auto& type = m_compiler.get_type(image.type_id);
        uint32_t set = m_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(image.id, spv::DecorationBinding);

        spdlog::info("Storage Image:{0}, Set:{1}, Binding:{2}", image.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Image " << image.name << " has array size: " << arrayLength << std::endl;
        }

        out.storageImages.push_back(ShaderResourceInfo{ image.name, stage, set, binding, 0, 0, arrayLength });
    }

    // 处理 seperate samplers (采样器) -- hlsl
    for (const auto& sampler : m_resources.separate_samplers) {
        auto& type = m_compiler.get_type(sampler.type_id);
        uint32_t set = m_compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(sampler.id, spv::DecorationBinding);

        spdlog::info("Separate Sampler:{0}, Set:{1}, Binding:{2}", sampler.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Sampler " << sampler.name << " has array size: " << arrayLength << std::endl;
        }

        out.seperateSamplers.push_back(ShaderResourceInfo{ sampler.name, stage, set, binding, 0, 0, arrayLength });
    }

    for (const auto& as : m_resources.acceleration_structures) {
        auto& type = m_compiler.get_type(as.type_id);
        uint32_t set = m_compiler.get_decoration(as.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(as.id, spv::DecorationBinding);

        spdlog::info("Accelaration Structure:{0}, Set:{1}, Binding:{2}", as.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
        }

        out.accelarationStructs.push_back(ShaderResourceInfo{ as.name, stage, set, binding, 0, 0, arrayLength });
    }

    return out;
}

ShaderReflectorOut MVulkanShaderReflector::GenerateShaderReflactorOut2()
{
    ShaderStageFlagBits stage;
    std::string entryPointName;

    ShaderReflectorOut out;
    auto shader_stage = m_compiler.get_entry_points_and_stages();
    for (const auto& _stage : shader_stage) {
        switch (_stage.execution_model) {
        case spv::ExecutionModel::ExecutionModelVertex:
            stage = ShaderStageFlagBits::VERTEX;
            break;
        case spv::ExecutionModelFragment:
            stage = ShaderStageFlagBits::FRAGMENT;
            break;
        case spv::ExecutionModelGLCompute:
            stage = ShaderStageFlagBits::COMPUTE;
            break;
        case spv::ExecutionModelGeometry:
            stage = ShaderStageFlagBits::GEOMETRY;
            break;
        case spv::ExecutionModelTaskEXT:
            stage = ShaderStageFlagBits::TASK;
            break;
        case spv::ExecutionModelMeshEXT:
            stage = ShaderStageFlagBits::MESH;
            break;
        default:
            stage = ShaderStageFlagBits::VERTEX;
            break;
        }
        entryPointName = _stage.name;
    }


    for (const auto& ub : m_resources.uniform_buffers) {
        auto& type = m_compiler.get_type(ub.type_id);
        // 获取 Descriptor Set 和绑定点
        uint32_t set = m_compiler.get_decoration(ub.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(ub.id, spv::DecorationBinding);
        size_t size = m_compiler.get_declared_struct_size(type);

        spdlog::info("Uniform Buffer:{0}, Set:{1}, Binding:{2}, size:{3}", ub.name, set, binding, size);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "UBO " << ub.name << " has array size: " << arrayLength << std::endl;
        }

        out.m_resources.push_back(ShaderResourceInfo{ ub.name, stage, set, binding, size, 0, arrayLength });
    }

    for (const auto& sb : m_resources.storage_buffers) {
        //if (sb.name.find("counter.var") != std::string::npos) {
        //    continue;
        //

        auto& type = m_compiler.get_type(sb.type_id);
        // 获取 Descriptor Set 和绑定点
        uint32_t set = m_compiler.get_decoration(sb.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(sb.id, spv::DecorationBinding);
        size_t size = m_compiler.get_declared_struct_size(type);

        spdlog::info("Storage Buffer:{0}, Set:{1}, Binding:{2}, size:{3}", sb.name, set, binding, size);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "UBO " << ub.name << " has array size: " << arrayLength << std::endl;
        }

        out.m_resources.push_back(ShaderResourceInfo{ sb.name, stage, set, binding, size, 0, arrayLength });
    }

    // 处理 sampled images (纹理) -- glsl
    for (const auto& image : m_resources.sampled_images) {
        auto& type = m_compiler.get_type(image.type_id);
        uint32_t set = m_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(image.id, spv::DecorationBinding);

        spdlog::info("Sampled Image:{0}, Set:{1}, Binding:{2}", image.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Image " << image.name << " has array size: " << arrayLength << std::endl;
        }

        out.m_resources.push_back(ShaderResourceInfo{ image.name, stage, set, binding, 0, 0, arrayLength });
    }

    // 处理 seperate images (纹理) -- hlsl
    for (const auto& image : m_resources.separate_images) {
        auto& type = m_compiler.get_type(image.type_id);
        uint32_t set = m_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(image.id, spv::DecorationBinding);

        spdlog::info("Separate Image:{0}, Set:{1}, Binding:{2}", image.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Image " << image.name << " has array size: " << arrayLength << std::endl;
        }

        out.m_resources.push_back(ShaderResourceInfo{ image.name, stage, set, binding, 0, 0, arrayLength });
    }

    //storage Images
    for (const auto& image : m_resources.storage_images) {
        auto& type = m_compiler.get_type(image.type_id);
        uint32_t set = m_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(image.id, spv::DecorationBinding);

        spdlog::info("Storage Image:{0}, Set:{1}, Binding:{2}", image.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Image " << image.name << " has array size: " << arrayLength << std::endl;
        }

        out.m_resources.push_back(ShaderResourceInfo{ image.name, stage, set, binding, 0, 0, arrayLength });
    }

    // 处理 seperate samplers (采样器) -- hlsl
    for (const auto& sampler : m_resources.separate_samplers) {
        auto& type = m_compiler.get_type(sampler.type_id);
        uint32_t set = m_compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(sampler.id, spv::DecorationBinding);

        spdlog::info("Separate Sampler:{0}, Set:{1}, Binding:{2}", sampler.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
            //std::cout << "Sampler " << sampler.name << " has array size: " << arrayLength << std::endl;
        }

        out.m_resources.push_back(ShaderResourceInfo{ sampler.name, stage, set, binding, 0, 0, arrayLength });
    }

    for (const auto& as : m_resources.acceleration_structures) {
        auto& type = m_compiler.get_type(as.type_id);
        uint32_t set = m_compiler.get_decoration(as.id, spv::DecorationDescriptorSet);
        uint32_t binding = m_compiler.get_decoration(as.id, spv::DecorationBinding);

        spdlog::info("Accelaration Structure:{0}, Set:{1}, Binding:{2}", as.name, set, binding);

        uint32_t arrayLength = 1;
        if (!type.array.empty()) {
            arrayLength = type.array[0];
        }

        out.m_resources.push_back(ShaderResourceInfo{ as.name, stage, set, binding, 0, 0, arrayLength });
    }

    return out;
}

std::vector<MVulkanDescriptorSetLayoutBinding> ShaderReflectorOut::GetBindings()
{
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings;

    for (const auto& info : uniformBuffers) {
        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = info.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.binding.descriptorCount = info.descriptorCount;
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(info.stage);
        binding.size = info.size;
        bindings.push_back(binding);
    }

    for (const auto& info : storageBuffers) {
        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = info.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        if (info.descriptorCount == 0) {
            binding.binding.descriptorCount = BindlessDescriptorCount;
        }
        else {
            binding.binding.descriptorCount = info.descriptorCount;
        }
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(info.stage);
        binding.size = info.size;
        bindings.push_back(binding);
    }

    for (const auto& info : combinedImageSamplers) {
        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = info.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        if (info.descriptorCount == 0) {
            binding.binding.descriptorCount = BindlessDescriptorCount;
        }
        else {
            binding.binding.descriptorCount = info.descriptorCount;
        }
        //binding.binding.descriptorCount = info.descriptorCount;
        binding.binding.pImmutableSamplers = nullptr;
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(info.stage);
        bindings.push_back(binding);
    }

    for (const auto& info : seperateImages) {
        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = info.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        if (info.descriptorCount == 0) {
            binding.binding.descriptorCount = BindlessDescriptorCount;
        }
        else {
            binding.binding.descriptorCount = info.descriptorCount;
        }
        //binding.binding.descriptorCount = info.descriptorCount;
        binding.binding.pImmutableSamplers = nullptr;
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(info.stage);
        bindings.push_back(binding);
    }

    for (const auto& info : storageImages) {
        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = info.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        if (info.descriptorCount == 0) {
            binding.binding.descriptorCount = BindlessDescriptorCount;
        }
        else {
            binding.binding.descriptorCount = info.descriptorCount;
        }
        //binding.binding.descriptorCount = info.descriptorCount;
        binding.binding.pImmutableSamplers = nullptr;
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(info.stage);
        bindings.push_back(binding);
    }

    for (const auto& info : seperateSamplers) {
        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = info.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        binding.binding.descriptorCount = info.descriptorCount;
        binding.binding.pImmutableSamplers = nullptr;
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(info.stage);
        bindings.push_back(binding);
    }

    for (const auto& info : accelarationStructs) {
        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = info.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        binding.binding.descriptorCount = info.descriptorCount;
        binding.binding.pImmutableSamplers = nullptr;
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(info.stage);
        bindings.push_back(binding);
    }

    return bindings;
}

std::vector<std::vector<MVulkanDescriptorSetLayoutBinding>> ShaderReflectorOut::GetBindings2()
{
    std::vector<std::vector<MVulkanDescriptorSetLayoutBinding>> res;

    for (const auto& resource:m_resources)
    {
        if (res.size() < resource.set)
        {
            res.resize(resource.set);
        }

        MVulkanDescriptorSetLayoutBinding binding{};
        binding.binding.binding = resource.binding;
        binding.binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        binding.binding.descriptorCount = resource.descriptorCount;
        binding.binding.pImmutableSamplers = nullptr;
        binding.binding.stageFlags = ShaderStageFlagBits2VkShaderStageFlagBits(resource.stage);
        res[resource.set].push_back(binding);
    }

    return res;
}

std::vector<MVulkanDescriptorSetLayoutBinding> RemoveRepeatedBindings(std::vector<MVulkanDescriptorSetLayoutBinding> bindings)
{
    std::vector<MVulkanDescriptorSetLayoutBinding> res;
    std::unordered_set<int> binding_set;
    for (auto binding : bindings) {
        int bindingPoint = binding.binding.binding;
        if (binding_set.find(bindingPoint) == binding_set.end()) {
            res.push_back(binding);
            binding_set.insert(bindingPoint);
        }
    }
    return res;
}
