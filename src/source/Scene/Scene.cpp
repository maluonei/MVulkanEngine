#include "Scene/Scene.hpp"
#include <spdlog/spdlog.h>

//void Scene::Load(const std::string& path)
//{
//    Assimp::Importer importer;
//
//    // 通过指定路径加载模型
//    const aiScene* scene = importer.ReadFile(path,
//        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
//
//    // 检查加载是否成功
//    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
//        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
//        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
//        return;
//    }
//
//    // 遍历场景中的所有网格
//    processNode(scene->mRootNode, scene);
//}
//
//void Scene::processNode(const aiNode* node, const aiScene* scene)
//{
//    spdlog::info("node.name: " + std::string(node->mName.C_Str()));
//    spdlog::info("node.mNumChildren: " + std::to_string(node->mNumChildren));
//
//    // 处理当前节点的所有网格
//    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
//        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//        processMesh(mesh, scene);
//    }
//
//    // 递归处理每个子节点
//    for (unsigned int i = 0; i < node->mNumChildren; i++) {
//        processNode(node->mChildren[i], scene);
//    }
//}
//
//void Scene::processMesh(const aiMesh* mesh, const aiScene* scene)
//{
//
//}

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

    keys.reserve(m_meshMap.size()); // 预留空间提高性能
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

        std::cout << cmd.indexCount << "," << cmd.instanceCount << "," << cmd.firstIndex << "," << cmd.vertexOffset << "," << cmd.firstInstance << "\n";
    }

    //return m_indirectCommands;
}
