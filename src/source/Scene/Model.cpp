#include "Scene/Model.hpp"
#include <spdlog/spdlog.h>

void Model::Load(const std::string& path)
{
    Assimp::Importer importer;

    // ͨ��ָ��·������ģ��
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    // �������Ƿ�ɹ�
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }

    // ���������е���������
    processNode(scene->mRootNode, scene);
}

void Model::processNode(const aiNode* node, const aiScene* scene)
{
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

void Model::processMesh(const aiMesh* mesh, const aiScene* scene)
{
    //std::vector<Vertex> vertices;
    //std::vector<unsigned int> indices;

    // ������������ж���
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

    // �������������
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
}
