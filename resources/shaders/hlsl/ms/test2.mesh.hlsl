struct MeshOutput {
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

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

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void main(
    uint       gtid : SV_GroupThreadID, 
    uint       gid  : SV_GroupID, 
    out indices uint3 triangles[128], 
    out vertices MeshOutput vertices[64]
) {

    // Must be called before writing the geometry output
    SetMeshOutputCounts(4, 2); // 3 vertices, 1 primitive

    if (gtid < 1) {
        triangles[0] = uint3(0, 1, 2);
        triangles[1] = uint3(1, 2, 3);

        float4x4 MVP = Cam.Projection * Cam.View * Cam.Model;

        float4 pos0 = float4(-0.5, -0.5, 0.5, 1.0);
        vertices[0].Position = float4(mul(Cam.MVP, pos0)); 
        //vertices[0].Position = float4(-0.5, -0.5, 0.5, 1.0);
        //vertices[0].Position = float4(mul(Cam.MVP, vertices[0].Position)); 
        vertices[0].Color = float3(1.0, 0.0, 0.0);

        float4 pos1 = float4(-0.5, 0.5, 0.5, 1.0);
        vertices[1].Position = float4(mul(Cam.MVP, pos1)); 
        //vertices[1].Position = float4(-0.5, 0.5, 0.5, 1.0);
        //vertices[1].Position = float4(mul(Cam.MVP, vertices[1].Position)); 
        vertices[1].Color = float3(0.0, 1.0, 0.0);

        float4 pos2 = float4(0.5, -0.5, 0.5, 1.0);
        vertices[2].Position = float4(mul(Cam.MVP, pos2)); 
        //vertices[2].Position = float4(0.5, -0.5, 0.5, 1.0);
        //vertices[2].Position = float4(mul(Cam.MVP, vertices[2].Position)); 
        vertices[2].Color = float3(0.0, 0.0, 1.0);

        float4 pos3 = float4(0.5,  0.5, 0.5, 1.0);
        vertices[3].Position = float4(mul(Cam.MVP, pos3)); 
        //vertices[3].Position = float4(0.5,  0.5, 0.5, 1.0);
        //vertices[3].Position = float4(mul(Cam.MVP, vertices[3].Position)); 
        vertices[3].Color = float3(1.0, 0.0, 1.0);
    }
}