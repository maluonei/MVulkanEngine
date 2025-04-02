#ifndef MESHLET_HPP
#define MESHLET_HPP

#include "meshoptimizer.h"
#include <iostream>
#include <vector>
#include "Scene/Scene.hpp"

//struct meshopt_Meshlet
//{
//	unsigned int vertex_offset;   //表示这个meshlet在meshVertexIndices数组中的起点
//	unsigned int triangle_offset; //表示这个meshlet在meshTriangles数组中的起
//
//	unsigned int vertex_count;    //表示这个meshlet包含的顶点数量，即确定了在meshVertxIndices数组中的终点。
//	unsigned int triangle_count;  //同理，表示这个meshlet包含的三角形顶点数量。
//};
//
struct MeshletAddon {
	uint32_t instanceIndex;
	uint32_t materialIndex;
	uint32_t padding0;
	uint32_t padding1;
};

class Meshlet {
public:
	template<typename T>
	void createMeshlets(std::vector<T>* vertices, std::vector<uint32_t>* indices) {
		std::vector<meshopt_Meshlet> meshlets;
		std::vector<uint32_t> meshletVertices;
		std::vector<uint8_t>  meshletTriangles;
		std::vector<uint32_t> meshletTrianglesU32;

		const size_t kMaxVertices = 64;
		const size_t kMaxTriangles = 124;
		const float kConeWeight = 0.0f;

		//meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());

		const size_t maxMeshlets = meshopt_buildMeshletsBound(indices->size(), kMaxVertices, kMaxTriangles);
		meshlets.resize(maxMeshlets);
		meshletVertices.resize(maxMeshlets * kMaxVertices);
		meshletTriangles.resize(maxMeshlets * kMaxTriangles * 3);

		size_t meshletCount = meshopt_buildMeshlets(
			meshlets.data(),
			meshletVertices.data(),
			meshletTriangles.data(),
			reinterpret_cast<const uint32_t*>(indices->data()),
			indices->size(),
			reinterpret_cast<const float*>(vertices->data()),
			vertices->size(),
			sizeof(T),
			kMaxVertices,
			kMaxTriangles,
			kConeWeight
		);

		auto& last = meshlets[meshletCount - 1];
		meshletVertices.resize(last.vertex_offset + last.vertex_count);
		meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));	//保证一定是4的倍数
		meshlets.resize(meshletCount);

		for (auto& m : meshlets) {
			uint32_t triangleOffset = static_cast<uint32_t>(meshletTrianglesU32.size());
			for (uint32_t i = 0; i < m.triangle_count; ++i) {

				uint32_t i0 = 3 * i + 0 + m.triangle_offset;
				uint32_t i1 = 3 * i + 1 + m.triangle_offset;
				uint32_t i2 = 3 * i + 2 + m.triangle_offset;

				uint8_t vIdx0 = meshletTriangles[i0];
				uint8_t vIdx1 = meshletTriangles[i1];
				uint8_t vIdx2 = meshletTriangles[i2];
				uint32_t packed =  ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
								   ((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
								   ((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);
				meshletTrianglesU32.push_back(packed);

			}
			m.triangle_offset = triangleOffset;
		}

		std::cout << "verticesNum:  " << vertices->size() << std::endl;
		std::cout << "meshletNum:  " << meshletCount << std::endl;
		std::cout << "meshletVerticesNum:  " << meshletVertices.size() << std::endl;
		std::cout << "meshletTrianglesNum:  " << meshletTriangles.size() << std::endl;
		std::cout << "meshletTrianglesU32Num:  " << meshletTrianglesU32.size() << std::endl;

		m_meshlets.insert(m_meshlets.end(), meshlets.begin(), meshlets.end());
		m_meshletVertices.insert(m_meshletVertices.end(), meshletVertices.begin(), meshletVertices.end());
		m_meshletTriangles.insert(m_meshletTriangles.end(), meshletTriangles.begin(), meshletTriangles.end());
		m_meshletTrianglesU32.insert(m_meshletTrianglesU32.end(), meshletTrianglesU32.begin(), meshletTrianglesU32.end());
	}

	template<typename T>
	void createMeshlets(
		std::vector<T>* vertices, 
		std::vector<uint32_t>* indices,
		int instanceId,
		int matId,
		int meshVertexOffset,
		int meshVertexIndexOffset,
		int meshIndexOffset,
		int& vertexCount,
		int& indexCound) {

		std::vector<meshopt_Meshlet> meshlets;
		std::vector<uint32_t> meshletVertices;
		std::vector<uint8_t>  meshletTriangles;
		std::vector<uint32_t> meshletTrianglesU32;


		const size_t kMaxVertices = 64;
		const size_t kMaxTriangles = 124;
		const float kConeWeight = 0.0f;

		//meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());

		const size_t maxMeshlets = meshopt_buildMeshletsBound(indices->size(), kMaxVertices, kMaxTriangles);
		meshlets.resize(maxMeshlets);
		meshletVertices.resize(maxMeshlets * kMaxVertices);
		meshletTriangles.resize(maxMeshlets * kMaxTriangles * 3);

		size_t meshletCount = meshopt_buildMeshlets(
			meshlets.data(),
			meshletVertices.data(),
			meshletTriangles.data(),
			reinterpret_cast<const uint32_t*>(indices->data()),
			indices->size(),
			reinterpret_cast<const float*>(vertices->data()),
			vertices->size(),
			sizeof(T),
			kMaxVertices,
			kMaxTriangles,
			kConeWeight
		);

		std::vector<glm::vec4> meshletBounds; // Storage for meshlet bounding spheres
		for (auto& meshlet : meshlets)
		{
			auto bounds = meshopt_computeMeshletBounds(
				&meshletVertices[meshlet.vertex_offset],                 // Meshlet's starting vertex index
				&meshletTriangles[meshlet.triangle_offset],              // Meshlet's starting triangle
				meshlet.triangle_count,                                  // Meshlet's triangle count
				reinterpret_cast<const float*>(vertices->data()),  // Pointer to vertex positions
				vertices->size(),                                  // Number of vertex positions
				sizeof(T));                                         // Vertex data stride, only poistion in this case
			meshletBounds.push_back(glm::vec4(bounds.center[0], bounds.center[1], bounds.center[2], bounds.radius));
		}


		auto& last = meshlets[meshletCount - 1];
		meshletVertices.resize(last.vertex_offset + last.vertex_count);
		meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));	//保证一定是4的倍数
		meshlets.resize(meshletCount);
		
		MeshletAddon addOn{
			.instanceIndex = (uint32_t)(instanceId),
			.materialIndex = (uint32_t)(matId),
			.padding0 = 0,
			.padding1 = 0
		};
		std::vector<MeshletAddon>	 addons(meshletVertices.size(), addOn);
		//addons.resize(meshletCount);

		for (auto& i : meshletVertices) {
			i += meshVertexIndexOffset;
		}

		for (auto& m : meshlets) {
			uint32_t triangleOffset = static_cast<uint32_t>(meshletTrianglesU32.size());
			for (uint32_t i = 0; i < m.triangle_count; ++i) {

				uint32_t i0 = 3 * i + 0 + m.triangle_offset;
				uint32_t i1 = 3 * i + 1 + m.triangle_offset;
				uint32_t i2 = 3 * i + 2 + m.triangle_offset;

				uint8_t vIdx0 = meshletTriangles[i0];
				uint8_t vIdx1 = meshletTriangles[i1];
				uint8_t vIdx2 = meshletTriangles[i2];
				uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
					((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
					((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);
				meshletTrianglesU32.push_back(packed);

			}
			m.triangle_offset = meshIndexOffset + triangleOffset;
			m.vertex_offset += meshVertexOffset;
		}

		vertexCount = meshletVertices.size();
		indexCound = meshletTrianglesU32.size();

		//std::cout << "verticesNum:  " << vertices->size() << std::endl;
		//std::cout << "meshletNum:  " << meshletCount << std::endl;
		//std::cout << "meshletVerticesNum:  " << meshletVertices.size() << std::endl;
		//std::cout << "meshletTrianglesNum:  " << meshletTriangles.size() << std::endl;
		//std::cout << "meshletTrianglesU32Num:  " << meshletTrianglesU32.size() << std::endl;

		m_meshlets.insert(m_meshlets.end(), meshlets.begin(), meshlets.end());
		m_meshletVertices.insert(m_meshletVertices.end(), meshletVertices.begin(), meshletVertices.end());
		m_meshletTriangles.insert(m_meshletTriangles.end(), meshletTriangles.begin(), meshletTriangles.end());
		m_meshletTrianglesU32.insert(m_meshletTrianglesU32.end(), meshletTrianglesU32.begin(), meshletTrianglesU32.end());
		m_addons.insert(m_addons.end(), addons.begin(), addons.end());
		m_meshletBounds.insert(m_meshletBounds.end(), meshletBounds.begin(), meshletBounds.end());
	}

	void createMeshlets(std::shared_ptr<Scene> scene) {
	
		auto numPrims = scene->GetNumPrimInfos();
		int meshIndexOffset = 0;
		int meshVertexOffset = 0;
		int meshVertexIndexOffset = 0;
		auto drawIndexedIndirectCommands = scene->GetIndirectDrawCommands();

		for(auto i=0;i<numPrims;i++){
			//to mesh = scene->GetMesh(scene->m_primInfos[i].mesh_id);
			auto indirectCommand = drawIndexedIndirectCommands[i];
			auto mesh = scene->GetMesh(scene->m_primInfos[i].mesh_id);

			auto vertices = mesh->vertices;
			auto indices = mesh->indices;
			//auto matId = scene->m_primInfos[i].material_id;
			auto matId = indirectCommand.firstInstance;
			auto meshId = indirectCommand.firstInstance;
			//auto mat = m_scene->GetMaterial(m_scene->m_primInfos[i].material_id);

			auto meshVertexOffset = m_meshletVertices.size();

			int vCount = 0;
			int iCount = 0;

			createMeshlets(&vertices, &indices, meshId, matId, meshVertexOffset, meshVertexIndexOffset, meshIndexOffset, vCount, iCount);

			meshVertexOffset += vCount;
			meshVertexIndexOffset += vertices.size();
			//meshVertexOffset += vertices.size();
			meshIndexOffset += iCount;
		}
	
	}

	void packTriangles();

public:

	std::vector<meshopt_Meshlet>	m_meshlets;
	std::vector<uint32_t>			m_meshletVertices;
	std::vector<uint8_t>			m_meshletTriangles;
	std::vector<uint32_t>			m_meshletTrianglesU32;

	std::vector<MeshletAddon>		m_addons;
	
	//std::vector<>
	std::vector<glm::vec4>			m_meshletBounds;
	//std::vector<glm::mat4>			m_modelMatrixs;


};



#endif // !MESHLET_HPP
