#pragma once
#ifndef SCENE_HPP
#define SCENE_HPP

#include <assimp/Importer.hpp>      // 包含Assimp的Importer
#include <assimp/scene.h>           // 包含Assimp的scene数据结构
#include <assimp/postprocess.h>     // 包含Assimp的后处理标志
#include <memory>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include "Material.hpp"
#include <spdlog/spdlog.h>
#include "Util.hpp"

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
    uint32_t matId;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class Light;

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

    inline void SetIndirectBuffer(std::shared_ptr<Buffer> indirectBuffer) { 
        m_indirectBuffer = indirectBuffer; 
    }
    inline std::shared_ptr<Buffer> GetIndirectBuffer() { return m_indirectBuffer; }

    inline void SetTotalVertexs(std::vector<Vertex> totalVertexs){m_totalVertexs = totalVertexs;}
    inline std::vector<Vertex> GetTotalVertexs(){return m_totalVertexs;}

    inline void SetTotalIndeices(std::vector<uint32_t> totalIndeices){m_totalIndeices = totalIndeices;}
    inline std::vector<uint32_t> GetTotalIndeices(){return m_totalIndeices;}

    void GenerateIndirectDrawData();
    std::vector<std::string> GetMeshNames();
    void GenerateIndirectDrawCommand();
    inline std::vector<VkDrawIndexedIndirectCommand> GetIndirectDrawCommands(){return m_indirectCommands;}

    inline void AddLight(std::string name, std::shared_ptr<Light> light){m_lightMap[name] = light;}
    inline std::shared_ptr<Light> GetLight(std::string name){return m_lightMap[name];}

    inline void AddMaterial(std::shared_ptr<PhongMaterial> material){m_materials.push_back(material);}
    inline std::shared_ptr<PhongMaterial> GetMaterial(int index){return m_materials[index];}

    //inline std::vector<std::shared_ptr<PhongMaterial>> GetMaterial(int index) { return m_materials[index]; }

    inline BoundingBox GetBoundingBox() const { return m_bbx; }

    void CalculateBB();

    void AddScene(std::shared_ptr<Scene> scene, glm::mat4 transform);

    void GenerateIndirectDataAndBuffers();
private:
    //void calculateBB();

    BoundingBox m_bbx;

    std::vector<Vertex>     m_totalVertexs;
    std::vector<uint32_t>   m_totalIndeices;
    std::shared_ptr<Buffer> m_indirectBuffer = nullptr;
    std::shared_ptr<Buffer> m_indirectVertexBuffer = nullptr;
    std::shared_ptr<Buffer> m_indirectIndexBuffer = nullptr;
    std::vector<VkDrawIndexedIndirectCommand> m_indirectCommands;

    std::unordered_map<std::string, std::shared_ptr<Mesh>>      m_meshMap;
    std::unordered_map<std::string, std::shared_ptr<Buffer>>    m_vertexBufferMap;
    std::unordered_map<std::string, std::shared_ptr<Buffer>>    m_indexBufferMap;
    std::vector<std::shared_ptr<PhongMaterial>>                 m_materials;

    std::unordered_map<std::string, std::shared_ptr<Light>>     m_lightMap;
};

#endif