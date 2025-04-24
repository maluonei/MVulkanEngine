#include "Scene/EmbreeScene.hpp"
#include "Scene/Scene.hpp"

void EmbreeScene::Build(std::shared_ptr<Mesh> mesh)
{
    RTCDevice device = rtcNewDevice(nullptr);
    if (!device) {
        throw std::runtime_error("Failed to create Embree device");
    }
    
    // ��������
    scene_ = rtcNewScene(device);
    device_ = device;
    
    buildBVH(mesh);
}

void EmbreeScene::Clean()
{
    rtcReleaseScene(scene_);
    rtcReleaseDevice(device_);
}

HitResult EmbreeScene::RayCast(const glm::vec3 origin, const const glm::vec3 direction)
{
    HitResult result;
    result.hit = false;
    RayTracingData data;
    data.traversable = rtcGetSceneTraversable(scene_);
    
    // ��ʼ�����߽ṹ (Embree4 ʹ�� RTCRayHit �ṹ)
    RTCRayHit rayhit;
    memset(&rayhit, 0, sizeof(rayhit));
    
    rayhit.ray.org_x = origin[0];
    rayhit.ray.org_y = origin[1];
    rayhit.ray.org_z = origin[2];
    rayhit.ray.dir_x = direction[0];
    rayhit.ray.dir_y = direction[1];
    rayhit.ray.dir_z = direction[2];
    rayhit.ray.tnear = 0.0f;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = -1;
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
    
    
    // ִ�й���׷��
    RTCRayQueryContext context;
    rtcInitRayQueryContext(&context);
    
    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    args.context = &context;
    args.flags = RTC_RAY_QUERY_FLAG_NONE;
    args.feature_mask = RTC_FEATURE_FLAG_TRIANGLE;
    
    rtcTraversableIntersect1(data.traversable, &rayhit, &args);
    //RayStats_addRay(stats);
    
    // ����Ƿ�����
    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        result.hit = true;
        result.distance = rayhit.ray.tfar;
        result.point[0] = origin[0] + direction[0] * result.distance;
        result.point[1] = origin[1] + direction[1] * result.distance;
        result.point[2] = origin[2] + direction[2] * result.distance;
        result.geomID = rayhit.hit.geomID;
        result.primID = rayhit.hit.primID;

        result.normal = glm::normalize(glm::vec3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z));
        result.outSide = glm::dot(result.normal, direction) < 0.0f;
    }
    
    return result;
}

void EmbreeScene::buildBVH(std::shared_ptr<Mesh> mesh)
{
    RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
    
    // ���ö������� (Embree4 ��Ҫ 16 �ֽڶ���)
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
        sizeof(Vertex), mesh->vertices.size());
    memcpy(vb, mesh->vertices.data(), mesh->vertices.size() * sizeof(Vertex));
    
    // ������������
    uint32_t* ib = (uint32_t*)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
        sizeof(uint32_t) * 3, mesh->indices.size());
    
    memcpy(ib, mesh->indices.data(), mesh->indices.size() * sizeof(uint32_t));
    
    // ��������������
    //for (size_t i = 0; i < mesh->indices.size(); i+=3) {
    //    ib[i + 0] = mesh->indices[i + 0];
    //    ib[i + 1] = mesh->indices[i + 1];
    //    ib[i + 2] = mesh->indices[i + 2];
    //}
    
    // �ύ������
    rtcCommitGeometry(geom);
    
    // �������帽�ӵ�����
    rtcAttachGeometry(scene_, geom);
    rtcReleaseGeometry(geom);
    
    // �ύ�����Թ��� BVH
    rtcCommitScene(scene_);
}
