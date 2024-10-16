#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>      // 包含Assimp的Importer
#include <assimp/scene.h>           // 包含Assimp的scene数据结构
#include <assimp/postprocess.h>     // 包含Assimp的后处理标志
#include <memory>
#include <glm/glm.hpp>
#include <vector>
//#include <filesystem>

//using fs = std::filesystem;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

static size_t VertexSize[] = {
    sizeof(Vertex::position),
    sizeof(Vertex::normal),
    sizeof(Vertex::texcoord),
};

struct Mesh {
    //glm::mat4 transform;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class Model {
public:
    void Load(const std::string& path);
    inline uint32_t VertexSize()const { return static_cast<uint32_t>(sizeof(Vertex) * vertices.size()); }
    inline uint32_t IndexSize()const { return static_cast<uint32_t>(sizeof(uint32_t) * indices.size()); }
    std::vector<Vertex> GetVertices() const { return vertices; }
    std::vector<uint32_t> GetIndices() const { return indices; }
    const Vertex* GetVerticesData() const { return vertices.data(); }
    const uint32_t* GetIndicesData() const { return indices.data(); }
private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    void processNode(const aiNode* node, const aiScene* scene);
    void processMesh(const aiMesh* mesh, const aiScene* scene);
};



#endif