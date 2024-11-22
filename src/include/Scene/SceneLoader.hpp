#pragma once
#ifndef SCENELOADER_HPP
#define SCENELOADER_HPP


#include <string>
#include "../Managers/Singleton.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

#include <assimp/Importer.hpp>      // ����Assimp��Importer
#include <assimp/scene.h>           // ����Assimp��scene���ݽṹ
#include <assimp/postprocess.h>     // ����Assimp�ĺ����־

#include <memory>


class Scene;

class SceneLoader : public Singleton<SceneLoader> {
public:
	SceneLoader();
	void Load(std::string path, Scene* scene);


private:
	static int g_meshId;
	std::string m_currentScenePath;

	void loadTexture(std::string path);

	void processTextures(const aiScene* aiscene);
	void processMaterials(const aiScene* aiscene, Scene* scene);
	void processNode(const aiNode* node, const aiScene* aiscene, Scene* scene);
	void processMesh(const aiMesh* mesh, const aiScene* aiscene, Scene* scene);
};



#endif