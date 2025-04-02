#define AS_GROUP_SIZE 32
#define MS_GROUP_SIZE 128

struct CameraProperties {
    float4x4 Model;
    float4x4 View;
    float4x4 Projection;
    float4x4 MVP;
};

//ConstantBuffer<CameraProperties> Cam : register(b0);

[[vk::binding(1, 0)]]
cbuffer ubo1 : register(b1)
{
    CameraProperties Cam;
};

struct Vertex {
    float3 Position;
    float u;
    float3 Normal;
    float v;
};

struct Meshlet {
	uint VertexOffset;
	uint TriangleOffset;
	uint VertexCount;
	uint TriangleCount;
};

struct MeshletAddon{
    uint        InstanceID;
    uint        MaterialID;
    uint padding0;
    uint padding1;
};

//struct Model{
//    float4x4 Model;
//}

[[vk::binding(2, 0)]]StructuredBuffer<Vertex>  Vertices                 : register(t1);
[[vk::binding(3, 0)]]StructuredBuffer<Meshlet> Meshlets                 : register(t2);
[[vk::binding(4, 0)]]StructuredBuffer<uint>    VertexIndices            : register(t3);
[[vk::binding(5, 0)]]StructuredBuffer<uint>    TriangleIndices          : register(t4);
[[vk::binding(6, 0)]]StructuredBuffer<MeshletAddon> MeshletsAddon       : register(t5);
//[[vk::binding(6, 0)]]StructuredBuffer<float4x4>    Models      : register(t5);
//[[vk::binding(5, 0)]]StructuredBuffer<CameraProperties>    MVPs : register(t5);

struct MeshOutput {
    float4 Position : SV_POSITION;
    uint4 InstanceIDAndMatletIDAndMaterialID : TEXCOORD0;
    //uint InstanceIDAndPrimitiveID : SV_TEXCOORD0;
    //uint MaterialID : SV_TEXCOORD0;
    //float3 Normal   : NORMAL;
    float3 Color    : COLOR;

};

struct ToFragmentPrimitive {
  uint primitive_id: SV_PrimitiveID;
};

struct Payload {
    uint MeshletIndices[AS_GROUP_SIZE];
    //uint ModelIndices[AS_GROUP_SIZE];
};


[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void main(
                 uint       gtid : SV_GroupThreadID, 
                 uint       gid  : SV_GroupID, 
    in payload   Payload    payload, 
    out indices  uint3      triangles[128], 
    out primitives ToFragmentPrimitive primitives[128],
    out vertices MeshOutput vertices[64]) 
{
    //uint instanceIndex = payload.ModelIndices[gid];
    uint meshletIndex = payload.MeshletIndices[gid];

    Meshlet m = Meshlets[meshletIndex];
    SetMeshOutputCounts(m.VertexCount, m.TriangleCount);
       
    if (gtid < m.TriangleCount) {
        uint index = m.TriangleOffset + gtid;
        uint packed = TriangleIndices[index];
        uint vIdx0  = (packed >>  0) & 0xFF;
        uint vIdx1  = (packed >>  8) & 0xFF;
        uint vIdx2  = (packed >> 16) & 0xFF;
        triangles[gtid] = uint3(vIdx0, vIdx1, vIdx2);
        primitives[gtid].primitive_id = gtid;
    }

    if (gtid < m.VertexCount) {
        uint vertexIndex = m.VertexOffset + gtid;
        uint matID = MeshletsAddon[vertexIndex].MaterialID;
        uint instanceID = MeshletsAddon[vertexIndex].InstanceID;

        vertexIndex = VertexIndices[vertexIndex];

        //float3 normal = Vertices[vertexIndex].Normal;
        float4 pos = float4(Vertices[vertexIndex].Position, 1.0);

        vertices[gtid].Position = mul(Cam.MVP, pos);

        //float4 worldPos = mul(Models[instanceIndex], pos);
        float4 worldPos = mul(Cam.Model, pos);
        float4 cameraSpacePos = mul(Cam.View, worldPos);
        float4 clipSpacePos = mul(Cam.Projection, cameraSpacePos);
        vertices[gtid].Position = clipSpacePos;

        vertices[gtid].InstanceIDAndMatletIDAndMaterialID = uint4(instanceID, meshletIndex, matID, 0);

        float3 color = float3(
            float(gid & 1),
            float(gid & 3) / 4,
            float(gid & 7) / 8);
        vertices[gtid].Color = color;
        //vertices[gtid].Normal = normal;

        
        //vertices[gtid].Color = (2 * normal - 1.f);
    }
}