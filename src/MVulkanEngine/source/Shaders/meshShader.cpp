#include "Shaders/meshShader.hpp"

TestMeshShader::TestMeshShader():
    MeshShaderModule("hlsl/ms/test.mesh.hlsl", "hlsl/ms/test.frag.hlsl", "main", "main", false)
{
}

MeshletShader::MeshletShader():
	MeshShaderModule("hlsl/ms/meshlet.mesh.hlsl", "hlsl/ms/test.frag.hlsl", "main", "main", false)
{
}

size_t MeshletShader::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(ubo0);
}

void MeshletShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<CameraProperties*>(data);
		return;
	}
}

void* MeshletShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	}
}

TestMeshShader2::TestMeshShader2():
	MeshShaderModule("hlsl/ms/test2.mesh.hlsl", "hlsl/ms/test.frag.hlsl", "main", "main", false)
{
}

size_t TestMeshShader2::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(ubo0);
}

void TestMeshShader2::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<CameraProperties*>(data);
		return;
	}
}

void* TestMeshShader2::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	}
}

TestMeshShader3::TestMeshShader3() :
	MeshShaderModule("hlsl/ms/test3.mesh.hlsl", "hlsl/ms/test.frag.hlsl", "main", "main", false)
{

}

size_t TestMeshShader3::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(ubo0);
}

void TestMeshShader3::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<TestMeshShader3::CameraProperties*>(data);
		return;
	}
}

void* TestMeshShader3::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	}
}
