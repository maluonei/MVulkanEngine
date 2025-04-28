//#pragma once
#ifndef EMBREE_SCENE_HPP
#define EMBREE_SCENE_HPP

#include <embree4/rtcore.h>
#include <memory>
#include "Util.hpp"
#include <vector>

struct Mesh;

struct RayTracingData {
	int max_path_length;

	/* scene data */
	RTCScene scene;
	RTCTraversable traversable;
};

struct HitResult {
    bool hit = false;
    float distance = 0.0f;
	glm::vec3 point;// [3] = { 0, 0, 0 };
	glm::vec3 normal;
    unsigned geomID = RTC_INVALID_GEOMETRY_ID;
    unsigned primID = RTC_INVALID_GEOMETRY_ID;
	bool outSide = false;
};

class EmbreeScene {
public:
	void Build(std::shared_ptr<Mesh> mesh);

	void Clean();

	HitResult RayCast(const glm::vec3 origin, const glm::vec3 direction);
	
	void buildBVH(std::shared_ptr<Mesh> mesh);
private:
	std::shared_ptr<Mesh> m_mesh;
	//std::vector<glm::vec3> positions;
	//std::vector<uint32_t> indices;

	RTCDevice device_;
	RTCScene scene_;

	static void FilterBackface(const RTCFilterFunctionNArguments* args);
	void handleBackface(const RTCFilterFunctionNArguments* args);
};


#endif