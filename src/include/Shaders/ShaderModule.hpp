#ifndef SHADERMODULE_H
#define SHADERMODULE_H

#include<glm/glm.hpp>
#include<string>
#include<unordered_map>
#include<MVulkanRHI/MVulkanShader.hpp>
#include<MVulkanRHI/MVulkanBuffer.hpp>
#include<MVulkanRHI/MVulkanSampler.hpp>

class ShaderModule {
public:
	ShaderModule(const std::string& vertPath, const std::string& fragPath);
	void Create(VkDevice device);
	void Clean(VkDevice device);

	inline MVulkanShader GetVertexShader() const{return vertShader;}
	inline MVulkanShader GetFragmentShader() const{return fragShader;}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t index, void* data);
	virtual void* GetData(uint32_t binding);

protected:

private:
	void load();

	MVulkanShader vertShader;
	MVulkanShader fragShader;

	std::string m_vertPath;
	std::string m_fragPath;

};

class TestShader : public ShaderModule {
private:
	struct UniformBufferObjectVert {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	} ubo0;

	struct UniformBufferObjectFrag {
		glm::vec3 color0;
		float padding0;
		glm::vec3 color1;
		float padding1;
	} ubo1;

	std::vector<uint32_t> sizes;

public:
	TestShader();
	virtual void SetUBO(uint8_t index, void* data);
	virtual inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizes[binding]; }
	virtual void* GetData(uint32_t binding);
};

class PhongShader : public ShaderModule {
private:
	struct UniformBufferObjectVert {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	} ubo0;

	struct UniformBufferObjectFrag {
		glm::vec3 cameraPosition;
		float padding0;
	} ubo1;

	std::vector<uint32_t> sizes;

public:
	PhongShader();

	virtual void SetUBO(uint8_t index, void* data);
	virtual inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizes[binding]; }
	virtual void* GetData(uint32_t binding);
};

class GbufferShader: public ShaderModule
{
public:
	GbufferShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t index, void* data);
	
	virtual void* GetData(uint32_t binding);
public:
	struct UniformBufferObject
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	};

private:
	UniformBufferObject ubo0;
};

class SquadPhongShader:public ShaderModule {
public:
	SquadPhongShader();
private:

};

#endif