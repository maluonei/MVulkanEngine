#ifndef HIZ_HPP
#define HIZ_HPP

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "MVulkanRHI/MVulkanCommand.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

class ComputePass;
//class StorageBuffer;
//class MVulkanTexture;
//class MVulkanTexture;

#ifndef MAX_HIZ_LAYERS
#define MAX_HIZ_LAYERS 13
#endif

class Hiz{
public:
	Hiz();

	void Clean();

	void UpdateHiz(glm::ivec2 basicResolution);
	//void UpdateHizRes(glm::ivec2 basicResolution);

	const int GetHizLayers()const;
	const glm::ivec2 GetHizRes(int layer)const;
	std::shared_ptr<MVulkanTexture> GetHizTexture();
	//VkImageView GetHizTexture(int layer);
	//std::vector<std::shared_ptr<MVulkanTexture>> GetHizTextures();
	std::vector<TextureSubResource> GetHizTextures();

	void Init(glm::ivec2 basicResolution);
	void Init(glm::ivec2 basicResolution, std::shared_ptr<MVulkanTexture> depth);
	void SetSourceDepth(std::shared_ptr<MVulkanTexture> depth);

	void Generate(MComputeCommandList commandList);
	void Generate(MComputeCommandList commandList, int& queryIndex);

	void Generate(
		MVulkanCommandQueue queue,
		MComputeCommandList commandList, 
		int& queryIndex);

	const int GetHizQueryIndexStart() const;
	const int GetHizQueryIndexEnd() const;
private:
	void updateHizRes(glm::ivec2 basicResolution);

	void initHizTextures();
	void initHizBuffers();
	void initComputePass();
	void initComputePass(std::shared_ptr<MVulkanTexture> depth);

	void initResetHizBufferPass();
	void initUpdateHizBufferPass();
	void initGenHizPass();

	void addHizImageBarrier(int layer);
	void addHizBufferReadBarrier();
	void addHizBufferWriteBarrier();
	//void copyHizToDepth();

private:
	int hizQueryIndexStart, hizQueryIndexEnd;

	int m_numHizLayers = 1;
	std::vector<glm::ivec2> m_hizRes;

	//std::vector<std::shared_ptr<MVulkanTexture>> m_hizTextures;
	std::shared_ptr<MVulkanTexture> m_hizTexture = nullptr;

	std::shared_ptr<StorageBuffer> m_hizBuffer;
	std::shared_ptr<StorageBuffer> m_hizDimensionsBuffer;
	std::shared_ptr<ComputePass> m_resetHizBufferPass;
	std::shared_ptr<ComputePass> m_updateHizBufferPass;
	std::shared_ptr<ComputePass> m_genHizPass;
};

#endif // !HIZ_HPP