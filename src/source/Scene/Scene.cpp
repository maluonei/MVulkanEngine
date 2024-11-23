#include "Scene/Scene.hpp"
#include <spdlog/spdlog.h>

//void Scene::Load(const std::string& path)
//{
//    Assimp::Importer importer;
//
//    // ͨ��ָ��·������ģ��
//    const aiScene* scene = importer.ReadFile(path,
//        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
//
//    // �������Ƿ�ɹ�
//    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
//        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
//        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
//        return;
//    }
//
//    // ���������е���������
//    processNode(scene->mRootNode, scene);
//}
//
//void Scene::processNode(const aiNode* node, const aiScene* scene)
//{
//    spdlog::info("node.name: " + std::string(node->mName.C_Str()));
//    spdlog::info("node.mNumChildren: " + std::to_string(node->mNumChildren));
//
//    // ����ǰ�ڵ����������
//    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
//        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//        processMesh(mesh, scene);
//    }
//
//    // �ݹ鴦��ÿ���ӽڵ�
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

        std::cout << cmd.indexCount << "," << cmd.instanceCount << "," << cmd.firstIndex << "," << cmd.vertexOffset << "," << cmd.firstInstance << "\n";
    }

    //return m_indirectCommands;
}
