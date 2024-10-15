#include "Scene/Model.hpp"
#include <spdlog/spdlog.h>

void Model::Load(const std::string& path)
{
    Assimp::Importer importer;

    // 通过指定路径加载模型
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    // 检查加载是否成功
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }

    // 遍历场景中的所有网格
    processNode(scene->mRootNode, scene);
}

void Model::processNode(const aiNode* node, const aiScene* scene)
{
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

void Model::processMesh(const aiMesh* mesh, const aiScene* scene)
{
    //std::vector<Vertex> vertices;
    //std::vector<unsigned int> indices;

    // 遍历网格的所有顶点
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) {
            vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else {
            vertex.texcoord = glm::vec2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }

    // 遍历网格的索引
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
}
