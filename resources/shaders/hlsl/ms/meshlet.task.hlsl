#define AS_GROUP_SIZE 32

struct Payload {
    uint MeshletIndices[AS_GROUP_SIZE];
    //uint mvpIndex;
};

groupshared Payload sPayload;

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
    sPayload.MeshletIndices[gtid] = dtid;
    // Assumes all meshlets are visible
    GroupMemoryBarrierWithGroupSync();
    if(gtid==1){
        printf("DispatchMesh, gtid==1");
        DispatchMesh(32, 1, 1, sPayload);
    
    }
}