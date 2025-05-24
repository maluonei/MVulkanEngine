#ifndef SHADER_MANAGER_HPP
#define SHADER_MANAGER_HPP

#include "Managers/Singleton.hpp"
#include <variant>
#include <unordered_map>
#include <spdlog/spdlog.h>

#include "MVulkanRHI/MVulkanBuffer.hpp"
//#include <>

class ShaderModule;
class ComputeShaderModule;
class MeshShaderModule;

enum class ShaderType {
    Generic,
    Mesh,
    Compute,
    Unknown
};

struct MShader {
    std::variant<
        std::shared_ptr<ShaderModule>,
        std::shared_ptr<ComputeShaderModule>,
        std::shared_ptr<MeshShaderModule>> m_shader;
};

class ShaderManager : public Singleton<ShaderManager> {
public:
    template<typename T>
    void AddShader(const std::string& name, std::shared_ptr<T> shader) {
        static_assert(
            std::is_same_v<T, ShaderModule> ||
            std::is_same_v<T, ComputeShaderModule> ||
            std::is_same_v<T, MeshShaderModule>,
            "Unsupported shader type"
            );

        if (m_shaderMap.find(name) == m_shaderMap.end())
        {
            m_shaderMap[name] = MShader{ shader };
        }
        else
        {
            spdlog::warn("shader name: {0} already in ShaderManager", name);
        }
    }

    void AddShader(const std::string& name, std::initializer_list<std::string> args, bool compileEveryTime = false);

    template<typename T>
    std::shared_ptr<T> GetShader(const std::string& name) const {
        auto it = m_shaderMap.find(name);
        if (it == m_shaderMap.end()) {
            spdlog::error("shader {0} not found in ShaderManager", name);
        	return nullptr;
        }
        const auto& shaderVariant = it->second.m_shader;
        if (std::holds_alternative<std::shared_ptr<T>>(shaderVariant)) {
            return std::get<std::shared_ptr<T>>(shaderVariant);
        }

        spdlog::error("shader {0} not found in ShaderManager", name);
        return nullptr;
    }

    const int GetNumShaders() const;

private:
    std::unordered_map<std::string, MShader> m_shaderMap;

private:
    virtual void InitSingleton();
    bool find_shader_num(std::initializer_list<std::string> args, int& numShaderFiles, ShaderType& type);
};

//struct ConstantBufferResource
//{
//    int binding;
//};

class ShaderResourceManager :public Singleton<ShaderResourceManager> {
public:

    void AddConstantBuffer(const std::string name, BufferCreateInfo info, int frameCount = 1);
    void LoadData(const std::string shaderResourceName, int frameIndex, void* data, int offset);
    MCBV GetBuffer(const std::string name, int frameIndex = 0);
    MCBV GetBuffer(const std::string name, const std::string shaderResourceName, int frameIndex = 0);
private:
    std::unordered_map<std::string, BufferCreateInfo> m_constantBufferCreateInfos;
    std::unordered_map<std::string, std::vector<MCBV>> m_constantBuffers;
    virtual void InitSingleton();
};

#endif

