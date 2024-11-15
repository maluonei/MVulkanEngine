#pragma once
#ifndef SCENE_HPP
#define SCENE_HPP

#include <assimp/Importer.hpp>      // 包含Assimp的Importer
#include <assimp/scene.h>           // 包含Assimp的scene数据结构
#include <assimp/postprocess.h>     // 包含Assimp的后处理标志
#include <memory>
#include <glm/glm.hpp>
#include <vector>

//struct Vertex {
//    glm::vec3 position;
//    glm::vec3 normal;
//    glm::vec2 texcoord;
//};
//
//static size_t VertexSize[] = {
//    sizeof(Vertex::position),
//    sizeof(Vertex::normal),
//    sizeof(Vertex::texcoord),
//};
//
//struct Mesh {
//    //glm::mat4 transform;
//    std::vector<Vertex> vertices;
//    std::vector<uint32_t> indices;
//};


class Scene {
public:
    void Load(const std::string& path);
private:
    void processNode(const aiNode* node, const aiScene* scene);
    void processMesh(const aiMesh* mesh, const aiScene* scene);
};

#endif