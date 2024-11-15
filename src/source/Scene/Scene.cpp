#include "Scene/Scene.hpp"
#include <spdlog/spdlog.h>

void Scene::Load(const std::string& path)
{
    Assimp::Importer importer;

    // ͨ��ָ��·������ģ��
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);

    // �������Ƿ�ɹ�
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }

    // ���������е���������
    processNode(scene->mRootNode, scene);
}

void Scene::processNode(const aiNode* node, const aiScene* scene)
{
    spdlog::info("node.name: " + std::string(node->mName.C_Str()));
    spdlog::info("node.mNumChildren: " + std::to_string(node->mNumChildren));

    // ����ǰ�ڵ����������
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }

    // �ݹ鴦��ÿ���ӽڵ�
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Scene::processMesh(const aiMesh* mesh, const aiScene* scene)
{

}
