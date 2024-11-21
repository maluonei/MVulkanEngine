#include "Scene/Model.hpp"
#include <spdlog/spdlog.h>

//void Model::Load(const std::string& path)
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
//void Model::CreateBuffers()
//{
//    uint32_t meshNum = meshes.size();
//}
//
//void Model::processNode(const aiNode* node, const aiScene* scene)
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
//void Model::processMesh(const aiMesh* mesh, const aiScene* scene)
//{
//    //std::vector<Vertex> vertices;
//    //std::vector<unsigned int> indices;
//
//    spdlog::info("mesh.name: " + std::string(mesh->mName.C_Str()));
//    spdlog::info("mesh.mNumVertices: " + std::to_string(mesh->mNumVertices));
//
//    
//    Mesh _mesh;
//
//    // �������������
//    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
//        aiFace face = mesh->mFaces[i];
//
//        for (unsigned int j = 0; j < face.mNumIndices; j++) {
//            _mesh.indices.push_back(face.mIndices[j]);
//            //spdlog::info("indices:" + std::to_string(face.mIndices[j] + currentVertexNum));
//        }
//    }
//
//    // ������������ж���
//    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
//        Vertex vertex;
//        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
//        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
//        if (mesh->mTextureCoords[0]) {
//            vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
//        }
//        else {
//            vertex.texcoord = glm::vec2(0.0f, 0.0f);
//        }
//        _mesh.vertices.push_back(vertex);
//    }
//
//    meshes.push_back(_mesh);
//}
