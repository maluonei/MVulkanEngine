#ifndef MESHLET_HPP
#define MESHLET_HPP

#include "meshoptimizer.h"
#include <iostream>
#include <vector>

//struct meshopt_Meshlet
//{
//	unsigned int vertex_offset;   //表示这个meshlet在meshVertexIndices数组中的起点
//	unsigned int triangle_offset; //表示这个meshlet在meshTriangles数组中的起
//
//	unsigned int vertex_count;    //表示这个meshlet包含的顶点数量，即确定了在meshVertxIndices数组中的终点。
//	unsigned int triangle_count;  //同理，表示这个meshlet包含的三角形顶点数量。
//};
//
class Meshlet {
public:
	template<typename T>
	void createMeshlets(std::vector<T>* vertices, std::vector<uint32_t>* indices) {

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
	}

	void packTriangles();

public:

	std::vector<meshopt_Meshlet> meshlets;
	std::vector<uint32_t> meshletVertices;
	std::vector<uint8_t>  meshletTriangles;
	std::vector<uint32_t> meshletTrianglesU32;
};



#endif // !MESHLET_HPP
