#ifndef SHADERMODULE_H
#define SHADERMODULE_H

#include<glm/glm.hpp>
#include<string>
#include<unordered_map>


class ShaderModule {
public:
	

private:


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
	void SetUBO0(glm::mat4 model, glm::mat4 view, glm::mat4 projection);
	void SetUBO1(glm::vec3 color0, glm::vec3 color1);
	inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizes[binding]; }
	void* GetData(uint32_t binding);
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
	void SetUBO0(glm::mat4 model, glm::mat4 view, glm::mat4 projection);
	void SetUBO1(glm::vec3 cameraPosition);
	inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizes[binding]; }
	void* GetData(uint32_t binding);
};

class GbufferShader
{
public:
	inline size_t GetBufferSizeBinding(uint32_t binding) const { return sizeof(UniformBufferObject); }
	inline void SetModel(const glm::mat4& model){ubo0.Model=model;}
	inline void SetView(const glm::mat4& view){ubo0.View=view;}
	inline void SetProjection(const glm::mat4& projection){ubo0.Projection=projection;}
	inline void SetUBO0(glm::mat4 model, glm::mat4 view, glm::mat4 projection)
	{
		ubo0.Model = model;
		ubo0.View = view;
		ubo0.Projection = projection;
	}
	
	void* GetData(uint32_t binding);
private:
	struct UniformBufferObject
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	}ubo0;
};

class SquadPhongShader {
public:
	inline size_t GetBufferSizeBinding(uint32_t binding) const { return 0; }
private:

};

#endif