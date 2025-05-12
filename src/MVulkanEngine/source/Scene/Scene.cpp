#include "Scene/Scene.hpp"
#include <spdlog/spdlog.h>
#include <MVulkanRHI/MVulkanEngine.hpp>
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include <Managers/TextureManager.hpp>
#include "MultiThread/ParallelFor.hpp"

Scene::~Scene() {

}

void Scene::Clean()
{
    for(auto mesh:m_meshs)
    {
        if(mesh->m_vertexBuffer)
        {
            mesh->m_vertexBuffer->Clean();
            mesh->m_indexBuffer->Clean();
        }
    }
    
    for (auto buffer : m_vertexBuffers) {
        buffer->Clean();
    }

    for (auto buffer : m_indexBuffers) {
        buffer->Clean();
    }

    m_indirectBuffer->Clean();
    m_indirectIndexBuffer->Clean();
    m_indirectVertexBuffer->Clean();
}

void Scene::GenerateIndirectDrawData()
{
    //std::vector<std::string> names = GetMeshNames();
    auto numMeshes = GetNumMeshes();
    auto vertexNum = 0;

    for (auto i = 0; i < numMeshes; i++) {

        auto mesh = m_meshs[i];
        for (auto vertex : mesh->vertices) {
            m_totalVertexs.push_back(vertex);
        }

        for (auto index : mesh->indices) {
            m_totalIndeices.push_back(index + vertexNum);
        }

        vertexNum = m_totalVertexs.size();
    }
}

//std::vector<std::string> Scene::GetMeshNames()
//{
//	std::vector<std::string> keys;
//
//    keys.reserve(m_meshMap.size()); // Ԥ���ռ��������
//    for (const auto& pair : m_meshMap) {
//        keys.push_back(pair.first);
//    }
//    return keys;
//}

void Scene::GenerateIndirectDrawCommand()
{
    m_indirectCommands.clear();

    auto numMeshes = GetNumMeshes();
    //std::vector<std::string> names = GetMeshNames();

    auto currentIndex = 0;
    auto firstInstance = 0;
    for (auto i = 0; i < numMeshes;i++) {
        //if (i != 6) continue;
        //auto name = names[i];
        auto meshIndex = m_primInfos[i][0].mesh_id;

        VkDrawIndexedIndirectCommand cmd{};
        cmd.indexCount = m_meshs[meshIndex]->indices.size();
        //if (i != 6) cmd.indexCount = 0;
        cmd.instanceCount = m_primInfos[i].size();
        cmd.firstIndex = currentIndex;
        cmd.vertexOffset = 0;
        cmd.firstInstance = firstInstance;
        firstInstance += cmd.instanceCount;

        m_indirectCommands.push_back(cmd);

        currentIndex += cmd.indexCount;

        //if (i == 0) break;
    }
}

void Scene::GenerateIndirectDrawCommand2()
{
    //m_indirectCommands.clear();
    //
    //auto numMeshes = GetNumMeshes();
    ////std::vector<std::string> names = GetMeshNames();
    //
    //auto currentIndex = 0;
    //for (auto i = 0; i < numMeshes; i++) {
    //    //if (i != 6) continue;
    //    //auto name = names[i];
    //    auto meshIndex = m_primInfos[i].mesh_id;
    //
    //    VkDrawIndexedIndirectCommand cmd{};
    //    cmd.indexCount = m_meshs[meshIndex]->indices.size();
    //    if (i != 6) cmd.indexCount = 0;
    //    cmd.instanceCount = 1;
    //    cmd.firstIndex = currentIndex;
    //    cmd.vertexOffset = 0;
    //    cmd.firstInstance = i;
    //
    //    m_indirectCommands.push_back(cmd);
    //
    //    currentIndex += m_meshs[meshIndex]->indices.size();
    //
    //    //if (i == 0) break;
    //}
}

void Scene::GenerateIndirectDrawCommand(int repeatNum)
{
    m_indirectCommands.clear();

    auto numMeshes = GetNumMeshes();
    //std::vector<std::string> names = GetMeshNames();

    //for (auto i = 0; i < repeatNum; i++) {
    //auto name = names[0];

    VkDrawIndexedIndirectCommand cmd{};
    cmd.indexCount = m_meshs[0]->indices.size();
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
    m_bbx.pMin = glm::vec3(10000.f);
    m_bbx.pMax = glm::vec3(-10000.f);

    for (auto mesh : m_meshs) {
        m_bbx.pMin.x = std::min(mesh->m_box.pMin.x, m_bbx.pMin.x);
        m_bbx.pMin.y = std::min(mesh->m_box.pMin.y, m_bbx.pMin.y);
        m_bbx.pMin.z = std::min(mesh->m_box.pMin.z, m_bbx.pMin.z);
        m_bbx.pMax.x = std::max(mesh->m_box.pMax.x, m_bbx.pMax.x);
        m_bbx.pMax.y = std::max(mesh->m_box.pMax.y, m_bbx.pMax.y);
        m_bbx.pMax.z = std::max(mesh->m_box.pMax.z, m_bbx.pMax.z);
    }
}

void Scene::AddScene(std::shared_ptr<Scene> scene, glm::mat4 transform)
{
    auto numMeshes = GetNumMeshes();
    //std::vector<std::string> names = scene->GetMeshNames();

    auto vertexNum = this->m_totalVertexs.size();

    for (auto i = 0; i < numMeshes; i++) {
        //auto name = names[i];

        auto mesh = scene->GetMesh(i);
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

    for (auto mesh : scene->m_meshs) {
        mesh->matId += baseMatId;
        this->m_meshs.push_back(mesh);
    }
}

void Scene::GenerateIndirectDataAndBuffers(int repeatNums)
{
    auto totalVertexs = GetTotalVertexs();
    auto totalIndeices = GetTotalIndices();

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
    auto totalIndeices = GetTotalIndices();

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

void Scene::GenerateIndirectDataAndBuffers2()
{
    auto totalVertexs = GetTotalVertexs();
    auto totalIndeices = GetTotalIndices();

    std::shared_ptr<Buffer> vertexBuffer = std::make_shared<Buffer>(BufferType::VERTEX_BUFFER);
    std::shared_ptr<Buffer> indexBuffer = std::make_shared<Buffer>(BufferType::INDEX_BUFFER);

    Singleton<MVulkanEngine>::instance().CreateBuffer(vertexBuffer, (const void*)(totalVertexs.data()), sizeof(Vertex) * totalVertexs.size());
    Singleton<MVulkanEngine>::instance().CreateBuffer(indexBuffer, (const void*)(totalIndeices.data()), sizeof(unsigned int) * totalIndeices.size());

    SetIndirectVertexBuffer(vertexBuffer);
    SetIndirectIndexBuffer(indexBuffer);

    GenerateIndirectDrawCommand2();
    std::vector<VkDrawIndexedIndirectCommand> commands = GetIndirectDrawCommands();
    std::shared_ptr<Buffer> indirectCommandBuffer = std::make_shared<Buffer>(BufferType::INDIRECT_BUFFER);
    Singleton<MVulkanEngine>::instance().CreateBuffer(indirectCommandBuffer, (const void*)(commands.data()), sizeof(VkDrawIndexedIndirectCommand) * commands.size());
    SetIndirectBuffer(indirectCommandBuffer);
}

void Scene::GenerateMeshBuffers()
{
    for(auto pair:m_meshs)
    {
        //auto name = pair.first;
        auto mesh = pair;

        mesh->m_vertexBuffer = std::make_shared<Buffer>(BufferType::VERTEX_BUFFER);
        mesh->m_indexBuffer = std::make_shared<Buffer>(BufferType::INDEX_BUFFER);
        
        Singleton<MVulkanEngine>::instance().CreateBuffer(mesh->m_vertexBuffer, (const void*)(mesh->vertices.data()), sizeof(Vertex) * mesh->vertices.size());
        Singleton<MVulkanEngine>::instance().CreateBuffer(mesh->m_indexBuffer, (const void*)(mesh->indices.data()), sizeof(unsigned int) * mesh->indices.size());
    }
}

int Scene::GetTotalPrimInfos() const
{
    int total = 0;
    auto numMeshs = m_primInfos.size();
    for (auto i = 0; i < numMeshs; i++) {
        auto numInstances = m_primInfos[i].size();
        total += numInstances;
    }

    return total;
}

std::vector<glm::mat4> Scene::GetTransforms()
{
    std::vector<glm::mat4> transforms(0);

    auto numMeshes = m_primInfos.size();
    for (auto i = 0; i < numMeshes; i++) {
        auto numInstances = m_primInfos[i].size();
        for (auto j = 0; j < numInstances; j++) {
            transforms.push_back(m_primInfos[i][j].transform);
        }
    }

    return transforms;
}

std::vector<MaterialBuffer> Scene::GetMaterials()
{
    auto numMaterials = m_materials.size();
    std::vector<MaterialBuffer> materials(numMaterials);

    for(auto i = 0; i < numMaterials; i++)
    {
        auto mat = m_materials[i];

        if (mat->diffuseTexture != "") {
            materials[i].diffuseTextureIdx = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
        }
        else {
            materials[i].diffuseTextureIdx = -1;
            materials[i].diffuseColor = glm::vec3(mat->diffuseColor.r, mat->diffuseColor.g, mat->diffuseColor.b);
        }

        if (mat->metallicAndRoughnessTexture != "") {
            materials[i].metallicAndRoughnessTextureIdx = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
        }
        else {
            materials[i].metallicAndRoughnessTextureIdx = -1;
        }
    }

    return materials;
}

std::vector<int> Scene::GetMaterialsIds()
{
    std::vector<int> materialsIds(0);

    auto numMeshes = m_primInfos.size();
    for (auto i = 0; i < numMeshes; i++) {
        auto numInstances = m_primInfos[i].size();
        for (auto j = 0; j < numInstances; j++) {
            materialsIds.push_back(m_primInfos[i][j].material_id);
        }
    }

    return materialsIds;
}

std::vector<InstanceBound> Scene::GetBounds()
{
    std::vector<InstanceBound> bounds;
    std::vector<CalculateTransformedInstanceBoundsTask> tasks;

    auto numMeshes = m_primInfos.size();
    for (auto i = 0; i < numMeshes; i++) {
        auto numInstances = m_primInfos[i].size();
        
        auto mesh = m_meshs[m_primInfos[i][0].mesh_id];
        auto bound = mesh->m_box;
        for (auto j = 0; j < numInstances; j++) {
            auto transform = m_primInfos[i][j].transform;
            tasks.push_back(CalculateTransformedInstanceBoundsTask(mesh, transform));
        }
    }

    ParallelFor(tasks.begin(), tasks.end(), [this](CalculateTransformedInstanceBoundsTask& task)
        {
            task.DoWork();
        },
        8
    );

    auto numInstances = tasks.size();
    bounds.resize(numInstances);
    for (auto i = 0; i < numInstances; i++) {
        auto& bbx = tasks[i].GetTransformedBoundingBox();
        bounds[i].center = bbx.GetCenter();
        bounds[i].extent = bbx.GetExtent();
    }

    return bounds;
}

//TexBuffer Scene::GenerateTexBuffer()
//{
//    TexBuffer texBuffer;
//
//    //auto meshNames = this->GetMeshNames();
//    auto numPrims = this->GetNumPrimInfos();
//    auto drawIndexedIndirectCommands = this->GetIndirectDrawCommands();
//
//    for (auto i = 0; i < numPrims; i++) {
//        //auto name = meshNames[i];
//        //auto mesh = this->GetMesh(i);
//        auto primInfo = this->m_primInfos[i];
//        auto mat = this->GetMaterial(primInfo.material_id);
//
//        auto indirectCommand = drawIndexedIndirectCommands[i];
//        if (mat->diffuseTexture != "") {
//            auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
//            texBuffer.tex[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
//        }
//        else {
//            texBuffer.tex[indirectCommand.firstInstance].diffuseTextureIdx = -1;
//            texBuffer.tex[indirectCommand.firstInstance].diffuseColor = glm::vec3(mat->diffuseColor.r, mat->diffuseColor.g, mat->diffuseColor.b);
//        }
//
//        if (mat->metallicAndRoughnessTexture != "") {
//            auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
//            texBuffer.tex[indirectCommand.firstInstance].metallicAndRoughnessTextureIdx = metallicAndRoughnessTexId;
//        }
//        else {
//            texBuffer.tex[indirectCommand.firstInstance].metallicAndRoughnessTextureIdx = -1;
//        }
//
//
//        if (mat->normalMap != "") {
//            auto normalmapIdx = Singleton<TextureManager>::instance().GetTextureId(mat->normalMap);
//            texBuffer.tex[indirectCommand.firstInstance].normalTextureIdx = normalmapIdx;
//        }
//        else {
//            texBuffer.tex[indirectCommand.firstInstance].normalTextureIdx = -1;
//        }
//    }
//
//    return texBuffer;
//}

glm::vec3 Scene::GetVertx(int geomIndex, int primIndex, int vertxIndex)
{
    auto mesh = m_meshs[geomIndex];
    auto triIndex = mesh->indices[3 * primIndex + vertxIndex];
    return mesh->positions[triIndex];
}

glm::vec3 Mesh::GetPosition(int triIndex, int vertxIndex)
{
    auto tri = indices[3 * triIndex + vertxIndex];
    return positions[tri];
}

CalculateTransformedInstanceBoundsTask::CalculateTransformedInstanceBoundsTask(std::shared_ptr<Mesh> mesh, glm::mat4 transform):
    m_mesh(mesh), m_transform(transform)
{
}

void CalculateTransformedInstanceBoundsTask::DoWork()
{
    glm::vec3 pMin = glm::vec3(10000.f);
    glm::vec3 pMax = glm::vec3(-10000.f);

    auto numPositions = m_mesh->positions.size();
    for (auto i = 0; i < numPositions; i++) {
        auto transformedPos = m_transform * glm::vec4(m_mesh->positions[i], 1.f);
        transformedPos /= transformedPos.w;

        pMin.x = std::min(transformedPos.x, pMin.x);
        pMin.y = std::min(transformedPos.y, pMin.y);
        pMin.z = std::min(transformedPos.z, pMin.z);
        pMax.x = std::max(transformedPos.x, pMax.x);
        pMax.y = std::max(transformedPos.y, pMax.y);
        pMax.z = std::max(transformedPos.z, pMax.z);
    }

    m_transformedBoundingBox.pMin = pMin;
    m_transformedBoundingBox.pMax = pMax;
}

const BoundingBox& CalculateTransformedInstanceBoundsTask::GetTransformedBoundingBox() const
{
    return m_transformedBoundingBox;
}
