#define AS_GROUP_SIZE 32

enum {
    FRUSTUM_PLANE_LEFT   = 0,
    FRUSTUM_PLANE_RIGHT  = 1,
    FRUSTUM_PLANE_TOP    = 2,
    FRUSTUM_PLANE_BOTTOM = 3,
    FRUSTUM_PLANE_NEAR   = 4,
    FRUSTUM_PLANE_FAR    = 5,
};

enum VisibilityFunc
{
    VISIBILITY_FUNC_NONE                = 0,
    VISIBILITY_FUNC_PLANES              = 1,
    VISIBILITY_FUNC_SPHERE              = 2,
    VISIBILITY_FUNC_CONE                = 3,
    VISIBILITY_FUNC_CONE_AND_NEAR_PLANE = 4,
};

struct FrustumPlane {
    float3 Normal;
    float  __pad0;
    float3 Position;
    float  __pad1;
};

struct FrustumCone {
    float3 Tip;
    float  Height;
    float3 Direction;
    float  Angle;
};

struct Payload {
    uint MeshletIndices[AS_GROUP_SIZE];
    //uint ModelIndices[AS_GROUP_SIZE];
    //uint mvpIndex;
};

groupshared Payload sPayload;

struct FrustumData {
    FrustumPlane  Planes[6];
    float4        Sphere;
    FrustumCone   Cone;
};

struct SceneProperties {
    //uint     InstanceCount;
    FrustumData Frustum;
    uint        MeshletCount;
    uint        VisibilityFunc;
};

struct Meshlet {
	uint VertexOffset;
	uint TriangleOffset;
	uint VertexCount;
	uint TriangleCount;
};

struct Model{
    uint        InstanceID;
    uint        MaterialID;
    uint padding0;
    uint padding1;
};

struct MeshletAddon{
    uint        InstanceID;
    uint        MaterialID;
    uint padding0;
    uint padding1;
};


[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    SceneProperties Scene;
};

[[vk::binding(2, 0)]] StructuredBuffer<float4>          MeshletBounds          : register(t0);
[[vk::binding(3, 0)]] StructuredBuffer<float4x4>        Models                 : register(t1);
[[vk::binding(4, 0)]] StructuredBuffer<Meshlet>         Meshlets               : register(t2);
[[vk::binding(5, 0)]] StructuredBuffer<MeshletAddon>    MeshletsAddon          : register(t3);
// -------------------------------------------------------------------------------------------------
// Amplification Shader
// -------------------------------------------------------------------------------------------------
float SignedPointPlaneDistance(float3 P, float3 planeN, float3 planeP)
{
    float d = dot(normalize(planeN), P - planeP);
    return d;
};

bool VisibleFrustumPlanes(float4 sphere)
{
    float d0 = SignedPointPlaneDistance(sphere.xyz, Scene.Frustum.Planes[0].Normal, Scene.Frustum.Planes[0].Position);
    float d1 = SignedPointPlaneDistance(sphere.xyz, Scene.Frustum.Planes[1].Normal, Scene.Frustum.Planes[1].Position);
    float d2 = SignedPointPlaneDistance(sphere.xyz, Scene.Frustum.Planes[2].Normal, Scene.Frustum.Planes[2].Position);
    float d3 = SignedPointPlaneDistance(sphere.xyz, Scene.Frustum.Planes[3].Normal, Scene.Frustum.Planes[3].Position);
    float d4 = SignedPointPlaneDistance(sphere.xyz, Scene.Frustum.Planes[4].Normal, Scene.Frustum.Planes[4].Position);
    float d5 = SignedPointPlaneDistance(sphere.xyz, Scene.Frustum.Planes[5].Normal, Scene.Frustum.Planes[5].Position);

    // Determine if we're on the positive half space of frustum planes
    bool pos0 = (d0 >= 0);
    bool pos1 = (d1 >= 0);
    bool pos2 = (d2 >= 0);
    bool pos3 = (d3 >= 0);
    bool pos4 = (d4 >= 0);
    bool pos5 = (d5 >= 0);

    bool inside = pos0 && pos1 && pos2 && pos3 && pos4 && pos5;
    return inside;
};

bool VisibleFrustumSphere(float4 sphere)
{
    // Intersection or inside with frustum sphere
    bool inside = (distance(sphere.xyz, Scene.Frustum.Sphere.xyz) < (sphere.w + Scene.Frustum.Sphere.w));
    return inside;
};

bool VisibleFrustumCone(float4 sphere)
{
    // Cone and sphere are within intersectable range
    float3 v0 = sphere.xyz - Scene.Frustum.Cone.Tip;
    float  d0 = dot(v0, Scene.Frustum.Cone.Direction);
    bool   i0 = (d0 <= (Scene.Frustum.Cone.Height + sphere.w));

    float cs = cos(Scene.Frustum.Cone.Angle * 0.5);
    float sn = sin(Scene.Frustum.Cone.Angle * 0.5);
    float a  = dot(v0, Scene.Frustum.Cone.Direction);
    float b  = a * sn / cs;
    float c  = sqrt(dot(v0, v0) - (a * a));
    float d  = c - b;
    float e  = d * cs;
    bool  i1 = (e < sphere.w);

    return i0 && i1;
};

bool VisibleFrustumConeAndNearPlane(float4 sphere) 
{
    bool i0 = VisibleFrustumCone(sphere);

    FrustumPlane frNear = Scene.Frustum.Planes[FRUSTUM_PLANE_NEAR];
    float d0 = SignedPointPlaneDistance(sphere.xyz, frNear.Normal, frNear.Position);
    bool  i1 = (abs(d0) < sphere.w); // Intersects with near plane
    bool  i2 = (d0 > 0);             // On positive half space of near plane

    return i0 && (i1 || i2);
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
        //visible = true;
        //sPayload.ModelIndices[gtid] = instanceIndex;
        Meshlet meshlet = Meshlets[meshletIndex];
        uint instanceIndex = MeshletsAddon[meshletIndex].InstanceID;
        //float4x4 ModelMatrix = Models[instanceIndex].M;

        sPayload.MeshletIndices[gtid]  = meshletIndex;

        float4x4 M = Models[instanceIndex];
        float4 meshletBoundingSphere = mul(M, float4(MeshletBounds[meshletIndex].xyz, 1.0));
        meshletBoundingSphere.w = MeshletBounds[meshletIndex].w;
        
        if (Scene.VisibilityFunc == VISIBILITY_FUNC_NONE) {
            visible = true;
        }
        else if (Scene.VisibilityFunc == VISIBILITY_FUNC_PLANES) {
            visible = VisibleFrustumPlanes(meshletBoundingSphere);
        }
        else if (Scene.VisibilityFunc == VISIBILITY_FUNC_SPHERE) {
            visible = VisibleFrustumSphere(meshletBoundingSphere);
        }
        else if (Scene.VisibilityFunc == VISIBILITY_FUNC_CONE) {
            visible = VisibleFrustumCone(meshletBoundingSphere);
        }
        else if (Scene.VisibilityFunc == VISIBILITY_FUNC_CONE_AND_NEAR_PLANE) {
            visible = VisibleFrustumConeAndNearPlane(meshletBoundingSphere);
        }        
        //visible = true;
    }

    uint visibleCount = WaveActiveCountBits(visible);

    //GroupMemoryBarrierWithGroupSync();
    if(gtid == 0)   
        DispatchMesh(visibleCount, 1, 1, sPayload);
}