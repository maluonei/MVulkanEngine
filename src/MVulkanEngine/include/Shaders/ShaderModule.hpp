#ifndef SHADERMODULE_H
#define SHADERMODULE_H

#include<glm/glm.hpp>
#include<string>
#include<unordered_map>
#include<MVulkanRHI/MVulkanShader.hpp>
#include<MVulkanRHI/MVulkanBuffer.hpp>
#include<MVulkanRHI/MVulkanSampler.hpp>

class Scene;
class ShaderModule{
public:
	ShaderModule(
		const std::string& vertPath, 
		const std::string& fragPath, 
		std::string vertEntry = "main",
		std::string fragEntry = "main",
		bool compileEveryTime = false);
	void Create(VkDevice device);
	void Clean();

	inline MVulkanShader GetVertexShader() const{return m_vertShader;}
	inline MVulkanShader GetFragmentShader() const{return m_fragShader;}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);
	const inline std::string GetVertEntryPoint() { return m_vertEntry; }
	const inline std::string GetFragEntryPoint() { return m_fragEntry; }

protected:
	bool m_compileEveryTime = false;

private:
	void load();

	MVulkanShader	m_vertShader;
	MVulkanShader	m_fragShader;

	std::string		m_vertPath;
	std::string		m_fragPath;

	std::string		m_vertEntry = "main";
	std::string		m_fragEntry = "main";
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
	virtual void SetUBO(uint8_t binding, void* data);
	virtual inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizes[binding]; }
	virtual void* GetData(uint32_t binding, uint32_t index = 0);
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

	virtual void SetUBO(uint8_t binding, void* data);
	virtual inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizes[binding]; }
	virtual void* GetData(uint32_t binding, uint32_t index = 0);
};

class GbufferShader: public ShaderModule
{
public:
	GbufferShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);
	
	virtual void* GetData(uint32_t binding, uint32_t index = 0);
public:
	struct UniformBufferObject0
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct UniformBuffer1 {
		int diffuseTextureIdx;
		int metallicAndRoughnessTexIdx;
		int matId;
		int normalMapIdx;

		glm::vec3 diffuseColor;
		int padding0;
	};

	struct UniformBufferObject1
	{
		UniformBuffer1 ubo[256];
	};

	static UniformBufferObject1 GetFromScene(const std::shared_ptr<Scene> scene);

private:
	UniformBufferObject0 ubo0;
	UniformBufferObject1 ubo1;
};

class SquadPhongShader:public ShaderModule {
public:
	SquadPhongShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct Light {
		glm::vec3 direction;
		float intensity;
		glm::vec3 color;
		int shadowMapIndex;

		glm::mat4 shadowViewProj;
	};

	struct DirectionalLightBuffer {
		Light lights[2];
		glm::vec3 cameraPos;
		int lightNum;
	};


private:
	DirectionalLightBuffer ubo0;
};


class LightingPbrShader :public ShaderModule {
public:
	LightingPbrShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct Light {
		glm::mat4 shadowViewProj;

		glm::vec3 direction;
		float intensity;

		glm::vec3 color;
		int shadowMapIndex;
		
		float cameraZnear;
		float cameraZfar;
		float padding6;
		float padding7;
	};

	struct UniformBuffer0 {
		Light lights[2];
		glm::vec3 cameraPos;
		int lightNum;

		int ResolusionWidth;
		int ResolusionHeight;
		int padding0;
		int padding1;
	};

private:
	UniformBuffer0 ubo0;
};



class LightingIBLShader :public ShaderModule {
public:
	LightingIBLShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct Light {
		glm::mat4 shadowViewProj;

		glm::vec3 direction;
		float intensity;

		glm::vec3 color;
		int shadowMapIndex;

		float cameraZnear;
		float cameraZfar;
		float padding6;
		float padding7;
	};

	struct UniformBuffer0 {
		Light lights[2];
		glm::vec3 cameraPos;
		int lightNum;

		int ResolusionWidth;
		int ResolusionHeight;
		int padding0;
		int padding1;
	};
private:
	UniformBuffer0 ubo0;
};


class ShadowShader :public ShaderModule {
public:
	ShadowShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct ShadowUniformBuffer {
		glm::mat4 shadowMVP;
	};
private:
	ShadowUniformBuffer ubo0;
};

class SkyboxShader :public ShaderModule {
public:
	SkyboxShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct UniformBuffer0 {
		glm::mat4 View;
		glm::mat4 Projection;
	};

private:
	UniformBuffer0 ubo0;
};

class IrradianceConvolutionShader :public ShaderModule {
public:
	IrradianceConvolutionShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct UniformBuffer0 {
		glm::mat4 View;
		glm::mat4 Projection;
	};

private:
	UniformBuffer0 ubo0;
};

class PreFilterEnvmapShader :public ShaderModule {
public:
	PreFilterEnvmapShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct UniformBuffer0 {
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct UniformBuffer1 {
		float roughness;
		float padding0;
		float padding1;
		float padding2;
	};

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

class IBLBrdfShader :public ShaderModule {
public:
	IBLBrdfShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);
};

class ComputeShaderModule {
public:
	ComputeShaderModule(
		const std::string& compPath, 
		std::string compEntry = "main",
		bool compileEveryTime = false);
	void Create(VkDevice device);
	void Clean();

	inline MVulkanShader GetComputeShader() const { return m_compShader; }

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);
	const inline std::string GetEntryPoint() { return m_entryPoint; }

protected:
	void load();

	MVulkanShader	m_compShader;
	bool			m_compileEveryTime = false;
	std::string		m_entryPoint = "main";

	std::string		m_compPath;
};

class TestSquadShader :public ShaderModule {
public:
	TestSquadShader();
};

class TestCompShader :public ComputeShaderModule {
public:
	struct Constants {
		uint32_t Width;
		uint32_t Height;
	};

	struct InputBuffer {
		float data[256 * 256];
	};

	struct OutputBuffer {
		float data[256 * 256];
	};

public:
	TestCompShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);


protected:
	Constants constant0;
	InputBuffer input1;
	OutputBuffer output2;
};

class LightingPbrSSRShader :public ShaderModule {
public:
	LightingPbrSSRShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct Light {
		glm::mat4 shadowViewProj;

		glm::vec3 direction;
		float intensity;

		glm::vec3 color;
		int shadowMapIndex;

		float cameraZnear;
		float cameraZfar;
		float padding6;
		float padding7;
	};

	struct UniformBuffer0 {
		Light lights[2];
		glm::vec3 cameraPos;
		int lightNum;

		int ResolusionWidth;
		int ResolusionHeight;
		int GbufferWidth;
		int GbufferHeight;
	};

private:
	UniformBuffer0 ubo0;
};

class SSRShader :public ShaderModule {
public:
	SSRShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct UniformBuffer0 {
		glm::mat4 viewProj;

		glm::vec3 cameraPos;
		float padding0;

		int GbufferWidth;
		int GbufferHeight;
		int padding1;
		int padding2;
	};

private:
	UniformBuffer0 ubo0;
};

#endif