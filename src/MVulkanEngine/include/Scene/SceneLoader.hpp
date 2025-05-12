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

glm::mat4 AIMat2GLMMat(aiMatrix4x4 transform);

class SceneLoader : public Singleton<SceneLoader> {
public:
	SceneLoader();
	void Load(std::string path, Scene* scene);
	void LoadForMeshlet(std::string path, Scene* scene);

	void SaveScene(std::string path);

protected:
	virtual void InitSingleton();
private:
	void load();

	static int g_meshId;
	std::string m_currentScenePath;

	void loadTexture(std::string path);

	void processTextures(const aiScene* aiscene);
	void processMaterials(const aiScene* aiscene, Scene* scene);
	//void processMaterial(const aiMaterial* material, const aiScene* aiscene, std::string matName);
	void processNode(const aiNode* node, const aiScene* aiscene, Scene* scene, glm::mat4 transform);
	void processMeshs(const aiScene* aiscene, Scene* scene);

	//std::unordered_map<std::string, std::shared_ptr<PhongMaterial>> materials;
};



#endif