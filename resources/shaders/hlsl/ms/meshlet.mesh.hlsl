struct CameraProperties {
    float4x4 Model;
    float4x4 View;
    float4x4 Projection;
    float4x4 MVP;
};

//ConstantBuffer<CameraProperties> Cam : register(b0);

[[vk::binding(0, 0)]]
cbuffer ubo1 : register(b1)
{
    CameraProperties Cam;
};

struct Vertex {
    float3 Position;
    float padiing0;
    //float3 Color;
    //float padiing1;
};

struct Meshlet {
	uint VertexOffset;
	uint TriangleOffset;
	uint VertexCount;
	uint TriangleCount;
};

[[vk::binding(1, 0)]]StructuredBuffer<Vertex>  Vertices        : register(t1);
[[vk::binding(2, 0)]]StructuredBuffer<Meshlet> Meshlets        : register(t2);
[[vk::binding(3, 0)]]StructuredBuffer<uint>    VertexIndices   : register(t3);
[[vk::binding(4, 0)]]StructuredBuffer<uint>    TriangleIndices : register(t4);

struct MeshOutput {
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void main(
                 uint       gtid : SV_GroupThreadID, 
                 uint       gid  : SV_GroupID, 
    out indices  uint3      triangles[128], 
    out vertices MeshOutput vertices[64]) 
{
    Meshlet m = Meshlets[gid];
    SetMeshOutputCounts(m.VertexCount, m.TriangleCount);
       
    if (gtid < m.TriangleCount) {
        uint index = m.TriangleOffset + gtid;
        uint packed = TriangleIndices[index];
        uint vIdx0  = (packed >>  0) & 0xFF;
        uint vIdx1  = (packed >>  8) & 0xFF;
        uint vIdx2  = (packed >> 16) & 0xFF;
        triangles[gtid] = uint3(vIdx0, vIdx1, vIdx2);
    }

    if (gtid < m.VertexCount) {
        uint vertexIndex = m.VertexOffset + gtid;        
        vertexIndex = VertexIndices[vertexIndex];

        float4 pos = float4(Vertices[vertexIndex].Position, 1.0);
        vertices[gtid].Position = mul(Cam.MVP, pos);

        float3 color = float3(
            float(gid & 1),
            float(gid & 3) / 4,
            float(gid & 7) / 8);
        vertices[gtid].Color = color;
    }
}
