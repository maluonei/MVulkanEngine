#define AS_GROUP_SIZE 32

struct Payload {
    uint MeshletIndices[AS_GROUP_SIZE];
    //uint ModelIndices[AS_GROUP_SIZE];
    //uint mvpIndex;
};

groupshared Payload sPayload;

struct SceneProperties {
    //uint     InstanceCount;
    uint        MeshletCount;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    SceneProperties Scene;
};

// -------------------------------------------------------------------------------------------------
// Amplification Shader
// -------------------------------------------------------------------------------------------------
[numthreads(AS_GROUP_SIZE, 1, 1)]
void main(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid  : SV_GroupID
)
{
    bool visible = false;

    //uint instanceIndex = dtid / Scene.MeshletCount;
    uint meshletIndex  = dtid % Scene.MeshletCount;

    if ((meshletIndex < Scene.MeshletCount)) {
        visible = true;
        //sPayload.ModelIndices[gtid] = instanceIndex;
        sPayload.MeshletIndices[gtid]  = meshletIndex;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    if(gtid == 0)   
        DispatchMesh(visibleCount, 1, 1, sPayload);
}