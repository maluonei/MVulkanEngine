#include "Scene/Scene.hpp"
#include <spdlog/spdlog.h>
#include <MVulkanRHI/MVulkanEngine.hpp>
#include "MVulkanRHI/MVulkanBuffer.hpp"

Scene::~Scene() {

}

void Scene::Clean()
{
    for(auto mesh:m_meshMap)
    {
        if(mesh.second->m_vertexBuffer)
        {
            mesh.second->m_vertexBuffer->Clean();
            mesh.second->m_indexBuffer->Clean();
        }
    }
    
    for (auto buffer : m_vertexBufferMap) {
        buffer.second->Clean();
    }

    for (auto buffer : m_indexBufferMap) {
        buffer.second->Clean();
    }

    m_indirectBuffer->Clean();
    m_indirectIndexBuffer->Clean();
    m_indirectVertexBuffer->Clean();
}

void Scene::GenerateIndirectDrawData()
{
    std::vector<std::string> names = GetMeshNames();
    auto vertexNum = 0;

    for (auto i = 0; i < names.size(); i++) {
        auto name = names[i];

        auto mesh = m_meshMap[name];
        for (auto vertex : mesh->vertices) {
            m_totalVertexs.push_back(vertex);
        }

        for (auto index : mesh->indices) {
            m_totalIndeices.push_back(index + vertexNum);
        }

        vertexNum = m_totalVertexs.size();
    }
}

std::vector<std::string> Scene::GetMeshNames()
{
	std::vector<std::string> keys;

    keys.reserve(m_meshMap.size()); // Ԥ���ռ��������
    for (const auto& pair : m_meshMap) {
        keys.push_back(pair.first);
    }
    return keys;
}

void Scene::GenerateIndirectDrawCommand()
{
    m_indirectCommands.clear();

    std::vector<std::string> names = GetMeshNames();

    auto currentIndex = 0;
    for (auto i = 0; i < names.size();i++) {
        auto name = names[i];

        VkDrawIndexedIndirectCommand cmd{};
        cmd.indexCount = m_meshMap[name]->indices.size();
        cmd.instanceCount = 1;
        cmd.firstIndex = currentIndex;
        cmd.vertexOffset = 0;
        cmd.firstInstance = i;

        m_indirectCommands.push_back(cmd);

        currentIndex += cmd.indexCount;
    }
}

void Scene::GenerateIndirectDrawCommand(int repeatNum)
{
    m_indirectCommands.clear();

    std::vector<std::string> names = GetMeshNames();

    //for (auto i = 0; i < repeatNum; i++) {
    auto name = names[0];

    VkDrawIndexedIndirectCommand cmd{};
    cmd.indexCount = m_meshMap[name]->indices.size();
    cmd.instanceCount = repeatNum;
    cmd.firstIndex = 0;
    cmd.vertexOffset = 0;
    cmd.firstInstance = 0;

    m_indirectCommands.push_back(cmd);

        //currentIndex += cmd.indexCount;
    //}
}

void Scene::CalculateBB()
{
    //BoundingBox bbx;
    m_bbx.pMin = glm::vec3(1000.f);
    m_bbx.pMax = glm::vec3(-1000.f);

    for (auto vertex : m_totalVertexs) {
        m_bbx.pMin.x = std::min(m_bbx.pMin.x, vertex.position.x);
        m_bbx.pMin.y = std::min(m_bbx.pMin.y, vertex.position.y);
        m_bbx.pMin.z = std::min(m_bbx.pMin.z, vertex.position.z);
        m_bbx.pMax.x = std::max(m_bbx.pMax.x, vertex.position.x);
        m_bbx.pMax.y = std::max(m_bbx.pMax.y, vertex.position.y);
        m_bbx.pMax.z = std::max(m_bbx.pMax.z, vertex.position.z);
    }
}

void Scene::AddScene(std::shared_ptr<Scene> scene, glm::mat4 transform)
{
    std::vector<std::string> names = scene->GetMeshNames();
    auto vertexNum = this->m_totalVertexs.size();

    for (auto i = 0; i < names.size(); i++) {
        auto name = names[i];

        auto mesh = scene->GetMesh(name);
        for (auto vertex : mesh->vertices) {
            glm::vec4 newPosition = transform * glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 1.f);
            newPosition /= newPosition.w;
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            glm::vec3 newNormal = glm::normalize(normalMatrix * vertex.normal);
           
            this->m_totalVertexs.push_back(Vertex{glm::vec3(newPosition.x, newPosition.y, newPosition.z), newNormal, vertex.texcoord});
        }

        for (auto index : mesh->indices) {
            this->m_totalIndeices.push_back(index + vertexNum);
        }

        vertexNum = this->m_totalVertexs.size();
    }

    auto baseMatId = m_materials.size();
    for (auto i = 0; i < scene->m_materials.size(); i++) {
        this->m_materials.push_back(scene->m_materials[i]);
    }

    for (auto mesh : scene->m_meshMap) {
        mesh.second->matId += baseMatId;
        this->m_meshMap.insert(std::make_pair(mesh.first, mesh.second));
    }
}

void Scene::GenerateIndirectDataAndBuffers(int repeatNums)
{
    auto totalVertexs = GetTotalVertexs();
    auto totalIndeices = GetTotalIndeices();

    std::shared_ptr<Buffer> vertexBuffer = std::make_shared<Buffer>(BufferType::VERTEX_BUFFER);
    std::shared_ptr<Buffer> indexBuffer = std::make_shared<Buffer>(BufferType::INDEX_BUFFER);

    Singleton<MVulkanEngine>::instance().CreateBuffer(vertexBuffer, (const void*)(totalVertexs.data()), sizeof(Vertex) * totalVertexs.size());
    Singleton<MVulkanEngine>::instance().CreateBuffer(indexBuffer, (const void*)(totalIndeices.data()), sizeof(unsigned int) * totalIndeices.size());

    SetIndirectVertexBuffer(vertexBuffer);
    SetIndirectIndexBuffer(indexBuffer);

    GenerateIndirectDrawCommand(repeatNums);
    std::vector<VkDrawIndexedIndirectCommand> commands = GetIndirectDrawCommands();
    std::shared_ptr<Buffer> indirectCommandBuffer = std::make_shared<Buffer>(BufferType::INDIRECT_BUFFER);
    Singleton<MVulkanEngine>::instance().CreateBuffer(indirectCommandBuffer, (const void*)(commands.data()), sizeof(VkDrawIndexedIndirectCommand) * commands.size());
    SetIndirectBuffer(indirectCommandBuffer);
}

void Scene::GenerateIndirectDataAndBuffers()
{
    auto totalVertexs = GetTotalVertexs();
    auto totalIndeices = GetTotalIndeices();

    std::shared_ptr<Buffer> vertexBuffer = std::make_shared<Buffer>(BufferType::VERTEX_BUFFER);
    std::shared_ptr<Buffer> indexBuffer = std::make_shared<Buffer>(BufferType::INDEX_BUFFER);

    Singleton<MVulkanEngine>::instance().CreateBuffer(vertexBuffer, (const void*)(totalVertexs.data()), sizeof(Vertex) * totalVertexs.size());
    Singleton<MVulkanEngine>::instance().CreateBuffer(indexBuffer, (const void*)(totalIndeices.data()), sizeof(unsigned int) * totalIndeices.size());

    SetIndirectVertexBuffer(vertexBuffer);
    SetIndirectIndexBuffer(indexBuffer);

    GenerateIndirectDrawCommand();
    std::vector<VkDrawIndexedIndirectCommand> commands = GetIndirectDrawCommands();
    std::shared_ptr<Buffer> indirectCommandBuffer = std::make_shared<Buffer>(BufferType::INDIRECT_BUFFER);
    Singleton<MVulkanEngine>::instance().CreateBuffer(indirectCommandBuffer, (const void*)(commands.data()), sizeof(VkDrawIndexedIndirectCommand) * commands.size());
    SetIndirectBuffer(indirectCommandBuffer);
}

void Scene::GenerateMeshBuffers()
{
    for(auto pair:m_meshMap)
    {
        auto name = pair.first;
        auto mesh = pair.second;

        mesh->m_vertexBuffer = std::make_shared<Buffer>(BufferType::VERTEX_BUFFER);
        mesh->m_indexBuffer = std::make_shared<Buffer>(BufferType::INDEX_BUFFER);
        
        Singleton<MVulkanEngine>::instance().CreateBuffer(mesh->m_vertexBuffer, (const void*)(mesh->vertices.data()), sizeof(Vertex) * mesh->vertices.size());
        Singleton<MVulkanEngine>::instance().CreateBuffer(mesh->m_indexBuffer, (const void*)(mesh->indices.data()), sizeof(unsigned int) * mesh->indices.size());
    }
}
