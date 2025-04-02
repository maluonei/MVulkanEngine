struct CameraProperties {
    float4x4 Model;
    float4x4 View;
    float4x4 Projection;
    float4x4 MVP;

    int numVertices;
    int numIndices;
    int padding0;
    int padding1;
};

//ConstantBuffer<CameraProperties> Cam : register(b0);

[[vk::binding(0, 0)]]
cbuffer ubo1 : register(b1)
{
    CameraProperties Cam;
};

struct Vertex {
    float3 Position;
    float padding0;
    float3 Color;
    float padding1;
};

[[vk::binding(1, 0)]]StructuredBuffer<Vertex>  Vertices        : register(t1);
[[vk::binding(2, 0)]]StructuredBuffer<uint>    Indices         : register(t2);

struct MeshOutput {
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void main(
                 uint       gtid : SV_GroupThreadID, 
                 uint       gid  : SV_GroupID, 
    out indices  uint3      triangles[128], 
    out vertices MeshOutput vertices[64]) 
{
    int numTriangles = Cam.numIndices / 3;
    SetMeshOutputCounts(Cam.numVertices, numTriangles); // 3 vertices, 1 primitive

    for(int i=0;i<numTriangles;i++){
        triangles[i] = uint3(Indices[i*3+0], Indices[i*3+1], Indices[i*3+2]);
    }

    for(int i=0;i<Cam.numVertices;i++){
        float3 pos = Vertices[i].Position;
        vertices[i].Position = mul(Cam.MVP, float4(pos, 1.0f));
        vertices[i].Color    = Vertices[i].Color;
    }
}
