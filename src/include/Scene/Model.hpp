#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>      // ����Assimp��Importer
#include <assimp/scene.h>           // ����Assimp��scene���ݽṹ
#include <assimp/postprocess.h>     // ����Assimp�ĺ����־
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
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class Model {
public:
    void Load(const std::string& path);
private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    void processNode(const aiNode* node, const aiScene* scene);
    void processMesh(const aiMesh* mesh, const aiScene* scene);
};



#endif