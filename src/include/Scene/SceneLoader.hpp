#pragma once
#ifndef SCENELOADER_HPP
#define SCENELOADER_HPP


#include <string>
#include "../Managers/Singleton.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

#include <assimp/Importer.hpp>      // 包含Assimp的Importer
#include <assimp/scene.h>           // 包含Assimp的scene数据结构
#include <assimp/postprocess.h>     // 包含Assimp的后处理标志

#include <memory>


class Scene;

class SceneLoader : public Singleton<SceneLoader> {
public:
	SceneLoader();
	void Load(std::string path, Scene* scene);


private:
	static int g_meshId;

	void processNode(const aiNode* node, const aiScene* aiscene, Scene* scene);
	void processMesh(const aiMesh* mesh, const aiScene* aiscene, Scene* scene);
};



#endif