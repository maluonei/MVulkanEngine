#include "Scene/Scene.hpp"
#include <spdlog/spdlog.h>

void Scene::Load(const std::string& path)
{
    Assimp::Importer importer;

    // 通过指定路径加载模型
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);

    // 检查加载是否成功
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }

    // 遍历场景中的所有网格
    processNode(scene->mRootNode, scene);
}

void Scene::processNode(const aiNode* node, const aiScene* scene)
{
    spdlog::info("node.name: " + std::string(node->mName.C_Str()));
    spdlog::info("node.mNumChildren: " + std::to_string(node->mNumChildren));

    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }

    // 递归处理每个子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Scene::processMesh(const aiMesh* mesh, const aiScene* scene)
{

}
