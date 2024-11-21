#pragma once
#ifndef SCENE_HPP
#define SCENE_HPP

#include <assimp/Importer.hpp>      // ����Assimp��Importer
#include <assimp/scene.h>           // ����Assimp��scene���ݽṹ
#include <assimp/postprocess.h>     // ����Assimp�ĺ����־
#include <memory>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <MVulkanRHI/MVulkanBuffer.hpp>
#include <spdlog/spdlog.h>

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


class Scene {
public:
    inline void SetMesh(std::string name, std::shared_ptr<Mesh> mesh){
        if (m_meshMap.find(name) != m_meshMap.end())
            spdlog::error("mesh: {} already exist", name);
        m_meshMap[name] = mesh;}
    inline std::shared_ptr<Mesh> GetMesh(std::string name){return m_meshMap[name];}

    inline void SetVertexBuffer(std::string name, std::shared_ptr<Buffer> vertexBuffer){m_vertexBufferMap[name] = vertexBuffer;}
    inline std::shared_ptr<Buffer> GetVertexBuffer(std::string name){return m_vertexBufferMap[name];}

    inline void SetIndexBuffer(std::string name, std::shared_ptr<Buffer> indexBuffer){m_indexBufferMap[name] = indexBuffer;}
    inline std::shared_ptr<Buffer> GetIndexBuffer(std::string name){return m_indexBufferMap[name];}

    inline void SetIndirectVertexBuffer(std::shared_ptr<Buffer> indirectVertexBuffer){m_indirectVertexBuffer = indirectVertexBuffer;}
    inline std::shared_ptr<Buffer> GetIndirectVertexBuffer(){return m_indirectVertexBuffer;}

    inline void SetIndirectIndexBuffer(std::shared_ptr<Buffer> indirectIndexBuffer){m_indirectIndexBuffer = indirectIndexBuffer;}
    inline std::shared_ptr<Buffer> GetIndirectIndexBuffer(){return m_indirectIndexBuffer;}

    inline void SetIndirectBuffer(std::shared_ptr<Buffer> indirectBuffer) { m_indirectBuffer = indirectBuffer; }
    inline std::shared_ptr<Buffer> GetIndirectBuffer() { return m_indirectBuffer; }

    inline void SetTotalVertexs(std::vector<Vertex> totalVertexs){m_totalVertexs = totalVertexs;}
    inline std::vector<Vertex> GetTotalVertexs(){return m_totalVertexs;}

    inline void SetTotalIndeices(std::vector<uint32_t> totalIndeices){m_totalIndeices = totalIndeices;}
    inline std::vector<uint32_t> GetTotalIndeices(){return m_totalIndeices;}

    void GenerateIndirectDrawData();
    std::vector<std::string> GetMeshNames();
    void GenerateIndirectDrawCommand();
    inline std::vector<VkDrawIndexedIndirectCommand> GetIndirectDrawCommands(){return m_indirectCommands;}
private:
    std::vector<Vertex> m_totalVertexs;
    std::vector<uint32_t> m_totalIndeices;
    std::shared_ptr<Buffer> m_indirectBuffer;
    std::shared_ptr<Buffer> m_indirectVertexBuffer;
    std::shared_ptr<Buffer> m_indirectIndexBuffer;
    std::vector<VkDrawIndexedIndirectCommand> m_indirectCommands;

    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshMap;
    std::unordered_map<std::string, std::shared_ptr<Buffer>> m_vertexBufferMap;
    std::unordered_map<std::string, std::shared_ptr<Buffer>> m_indexBufferMap;
};

#endif