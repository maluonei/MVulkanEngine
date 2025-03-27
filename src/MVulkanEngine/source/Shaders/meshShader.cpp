#include "Shaders/meshShader.hpp"

TestMeshShader::TestMeshShader():
    MeshShaderModule("hlsl/ms/test.mesh.hlsl", "hlsl/ms/test.frag.hlsl", "main", "main", false)
{
}
