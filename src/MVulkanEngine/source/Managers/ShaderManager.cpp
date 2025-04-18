#include "Managers/ShaderManager.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Shaders/ShaderModule.hpp"

void ShaderManager::AddShader(const std::string& name, std::initializer_list<std::string> args, bool compileEveryTime)
{
    auto argSize = args.size();

    ShaderType type = ShaderType::Unknown;
    int numShaderFiles;
    auto iter = args.begin();

    if (find_shader_num(args, numShaderFiles, type))
    {
        switch (type)
        {
        case ShaderType::Generic:
            //no geometry shader
            if (numShaderFiles == 2)
            {
                auto vertShaderFile = *iter++;
                auto fragShaderFile = *iter++;
                std::string vertEntry = "main";
                std::string fragEntry = "main";
                if (iter != args.end())
                {
                    vertEntry = *iter++;
                    fragEntry = *iter++;
                }

                std::shared_ptr<ShaderModule> shader =
                    std::make_shared<ShaderModule>(vertShaderFile, "", fragShaderFile, vertEntry, "", fragEntry, compileEveryTime);

                AddShader(name, shader);
            }
            else if (numShaderFiles == 3)
            {
                auto vertShaderFile = *iter++;
                auto geomShaderFile = *iter++;
                auto fragShaderFile = *iter++;
                std::string vertEntry = "main";
                std::string geomEntry = "main";
                std::string fragEntry = "main";
                if (iter != args.end())
                {
                    vertEntry = *iter++;
                    geomEntry = *iter++;
                    fragEntry = *iter++;
                }

                std::shared_ptr<ShaderModule> shader =
                    std::make_shared<ShaderModule>(vertShaderFile, geomShaderFile, fragShaderFile, vertEntry, geomEntry, fragEntry, compileEveryTime);

                AddShader(name, shader);
            }
            break;
        case ShaderType::Mesh:
            //no task shader
            if (numShaderFiles == 2)
            {
                auto meshShaderFile = *iter++;
                auto fragShaderFile = *iter++;
                std::string meshEntry = "main";
                std::string fragEntry = "main";
                if (iter != args.end())
                {
                    meshEntry = *iter++;
                    fragEntry = *iter++;
                }

                std::shared_ptr<MeshShaderModule> shader =
                    std::make_shared<MeshShaderModule>("", meshShaderFile, fragShaderFile, "", meshEntry, fragEntry, compileEveryTime);

                AddShader(name, shader);
            }
            else if (numShaderFiles == 3)
            {
                auto taskShaderFile = *iter++;
                auto meshShaderFile = *iter++;
                auto fragShaderFile = *iter++;
                std::string taskEntry = "main";
                std::string meshEntry = "main";
                std::string fragEntry = "main";
                if (iter != args.end())
                {
                    taskEntry = *iter++;
                    meshEntry = *iter++;
                    fragEntry = *iter++;
                }

                std::shared_ptr<ShaderModule> shader =
                    std::make_shared<ShaderModule>(taskShaderFile, meshShaderFile, fragShaderFile, taskEntry, meshEntry, fragEntry, compileEveryTime);

                AddShader(name, shader);
            }
            break;
        case ShaderType::Compute:
            auto compShaderFile = *iter++;
            std::string compEntry = "main";
            if (iter != args.end())
            {
                compEntry = *iter++;
            }

            std::shared_ptr<ComputeShaderModule> shader =
                std::make_shared<ComputeShaderModule>(compShaderFile, compEntry, compileEveryTime);

            AddShader(name, shader);
            break;
        //default:
        //    spdlog::error("unknown shader type");
        }
    }

    return;
}

const int ShaderManager::GetNumShaders() const
{
    return m_shaderMap.size();
}

void ShaderManager::InitSingleton()
{
	Singleton<ShaderManager>::InitSingleton();
}

bool ShaderManager::find_shader_num(std::initializer_list<std::string> args, int& numShaderFiles, ShaderType& type)
{
    numShaderFiles = 0;
    type = ShaderType::Unknown;

    int numArgs = args.size();
    auto it = args.begin();

    while (it != args.end())
    {
        auto str = *it;
        if (it->ends_with(".hlsl") || it->ends_with(".glsl"))
        {
            if (numShaderFiles == 0)
            {
                int length = str.size();
                if (length<9)
                {
                    spdlog::error("error shader name");
                }
                auto shaderType = str.substr(length-9, 4);
                if (shaderType =="vert" )
                {
                    type = ShaderType::Generic;
                }
                else if (shaderType == "task" || shaderType == "mesh")
                {
                    type = ShaderType::Mesh;
                }
                else if (shaderType == "comp")
                {
                    type = ShaderType::Compute;
                }
                else
                {
                    type = ShaderType::Unknown;
                    spdlog::error("unknown shader type");
                    return false;
                }
            }

            numShaderFiles++;
            it++;
            //break;
        }
        else
        {
            break;
        }
    }

    bool valid = false;

    switch (type)
    {
    case ShaderType::Generic:
        if (numShaderFiles == 2 || numShaderFiles == 3)
            valid = true;
        break;
    case ShaderType::Mesh:
        if (numShaderFiles == 2 || numShaderFiles == 3)
            valid = true;
        break;
    case ShaderType::Compute:
        if (numShaderFiles == 1)
            valid = true;
        break;
    }

    return valid;
}

void ShaderResourceManager::AddConstantBuffer(const std::string name, BufferCreateInfo info, int frameCount)
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    if (m_constantBuffers.find(name) == m_constantBuffers.end())
    {
        for (int i = 0; i < frameCount; i++) {
			MCBV mcbv;
			mcbv.Create(device, info);
            m_constantBuffers[name].emplace_back(mcbv);
        }
    }
    else
    {
        spdlog::error("Constant Buffer Rename: {0}", name);
    }
}

void ShaderResourceManager::LoadData(const std::string name, int frameIndex, void* data, int offset)
{
    if (m_constantBuffers.find(name) == m_constantBuffers.end())
    {
        spdlog::error("Constant Buffer:{0} not found", name);
    }
    else
    {
        auto buffers = m_constantBuffers[name];
        if (frameIndex > buffers.size())
        {
            spdlog::error("invalid index: {0}", frameIndex);
        }

        buffers[frameIndex].UpdateData(offset, data);
    }
}

MCBV ShaderResourceManager::GetBuffer(const std::string name, int frameIndex)
{
    if (m_constantBuffers.find(name) == m_constantBuffers.end())
    {
        spdlog::error("Constant Buffer:{0} not found", name);
    }
    else
    {
        auto& buffers = m_constantBuffers[name];
        if (frameIndex > buffers.size())
        {
            spdlog::error("invalid index: {0}", frameIndex);
        }

        return buffers[frameIndex];
    }
}

void ShaderResourceManager::InitSingleton()
{
	//Singleton<ShaderResourceManager>::InitSingleton();
}
