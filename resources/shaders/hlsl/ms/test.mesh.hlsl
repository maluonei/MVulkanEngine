struct MeshOutput {
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
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

    triangles[0] = uint3(0, 1, 2);
    triangles[1] = uint3(1, 2, 3);

    vertices[0].Position = float4(-0.5, -0.5, 0.5, 1.0);
    vertices[0].Color = float3(1.0, 0.0, 0.0);

    vertices[1].Position = float4(-0.5, 0.5, 0.5, 1.0);
    vertices[1].Color = float3(0.0, 1.0, 0.0);

    vertices[2].Position = float4(0.5, -0.5, 0.5, 1.0);
    vertices[2].Color = float3(0.0, 0.0, 1.0);

    vertices[3].Position = float4(0.5,  0.5, 0.5, 1.0);
    vertices[3].Color = float3(1.0, 0.0, 1.0);
}