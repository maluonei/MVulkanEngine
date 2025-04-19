#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Managers/TextureManager.hpp"
#include <spdlog/spdlog.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

int SceneLoader::g_meshId = 0;

SceneLoader::SceneLoader()
{
   
}


void SceneLoader::Load(std::string path, Scene* scene)
{
    if (scene == nullptr) {
        spdlog::error("Scene is nullptr");
    }

    m_currentScenePath = path;

    Assimp::Importer importer;

    spdlog::info("assimp start loading");

    // 通过指定路径加载模型
    const aiScene* aiscene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_GenNormals
        //aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices | aiProcess_GenNormals
    );

    //const aiScene* aiscene = importer.ReadFile(path,
    //    aiProcess_JoinIdenticalVertices
    //    //aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices | aiProcess_GenNormals
    //);

    // 检查加载是否成功
    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }
    
    spdlog::info("start processing scene");
    processMeshs(aiscene, scene);
    processTextures(aiscene);
    processMaterials(aiscene, scene);

    processNode(aiscene->mRootNode, aiscene, scene);

    scene->GenerateIndirectDrawData();

    scene->CalculateBB();
}

void SceneLoader::LoadForMeshlet(std::string path, Scene* scene)
{
    if (scene == nullptr) {
        spdlog::error("Scene is nullptr");
    }

    m_currentScenePath = path;

    //Assimp::Importer importer;
    //
    //spdlog::info("assimp start loading");
    //
    //// 通过指定路径加载模型
    ////const aiScene* aiscene = importer.ReadFile(path,
    ////    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_GenNormals
    ////    //aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices | aiProcess_GenNormals
    ////);
    //
    //const aiScene* aiscene = importer.ReadFile(path,
    //    0
    //    //aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices | aiProcess_GenNormals
    //);
    //
    //// 检查加载是否成功
    //if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
    //    //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
    //    spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
    //    return;
    //}
    //
    //spdlog::info("start processing scene");
    //processMeshs(aiscene, scene);
    //processTextures(aiscene);
    //processMaterials(aiscene, scene);
    //
    //processNode(aiscene->mRootNode, aiscene, scene);

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    //std::string warn;
    std::string err;

    bool        loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(), nullptr, false);

    if (!loaded || !err.empty())
    {
        spdlog::error("fail to load obj model: {0}", path.c_str());
        throw std::exception();
    }

    size_t numShapes = shapes.size();
    if (numShapes == 0)
    {
        spdlog::error("model.numShapes = 0");
        throw std::exception();
    }

    // Make sure all vertex positions are 3 components wide
    if ((attrib.vertices.size() % 3) != 0)
    {
        spdlog::error("model.vertices.size is not a multiple of 3");
        throw std::exception();
    }

    // If there's normals and tex coords, they need to line up with the vertex positions
    const size_t positionCount = attrib.vertices.size() / 3;
    const size_t normalCount = attrib.normals.size() / 3;
    const size_t texCoordCount = attrib.texcoords.size() / 2;
    if ((normalCount > 0) && (normalCount != positionCount))
    {
        spdlog::warn("model don't have normal data");
    }
    if ((texCoordCount > 0) && (texCoordCount != positionCount))
    {
        spdlog::warn("model don't have uv data");
    }


    {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

        auto indicesNum = 0;
        for (size_t shapeIdx = 0; shapeIdx < numShapes; ++shapeIdx)
        {
            auto& shape = shapes[shapeIdx];

            indicesNum += shape.mesh.indices.size();
        }

        mesh->vertices.resize(positionCount);
        mesh->indices.resize(indicesNum);

        auto positions = reinterpret_cast<const glm::vec3*>(attrib.vertices.data());

        for (int i = 0; i < positionCount; i++) {
            Vertex vertex;
            vertex.position = positions[i];
            mesh->vertices[i] = vertex;
        }

        int indexid = 0;
        for (size_t shapeIdx = 0; shapeIdx < numShapes; ++shapeIdx)
        {
            auto& shape = shapes[shapeIdx];

            size_t numTriangles = shape.mesh.indices.size() / 3;
            for (size_t triIdx = 0; triIdx < numTriangles; ++triIdx)
            {
                uint32_t vIdx0 = static_cast<uint32_t>(shape.mesh.indices[3 * triIdx + 0].vertex_index);
                uint32_t vIdx1 = static_cast<uint32_t>(shape.mesh.indices[3 * triIdx + 1].vertex_index);
                uint32_t vIdx2 = static_cast<uint32_t>(shape.mesh.indices[3 * triIdx + 2].vertex_index);
                mesh->indices[indexid++] = vIdx0;
                mesh->indices[indexid++] = vIdx1;
                mesh->indices[indexid++] = vIdx2;
            }
        }

        scene->SetNumMeshes(1);
        scene->SetMesh(0, mesh);
    }

    scene->GenerateIndirectDrawData();

    scene->CalculateBB();
}

void SceneLoader::InitSingleton()
{
}

//void SceneLoader::processMaterial(const aiMaterial* material, const aiScene* aiscene, std::string matName)
//{
//
//}

void SceneLoader::processNode(const aiNode* node, const aiScene* aiscene, Scene* scene)
{
    //spdlog::info("node.name: " + std::string(node->mName.C_Str()));
    //spdlog::info("node.mNumChildren: " + std::to_string(node->mNumChildren));

    if (node->mMeshes) {
        for (auto i = 0; i < node->mNumMeshes; i++) {
            const auto meshIndex = node->mMeshes[i];
            //if (meshIndex == 0) continue;
            auto* const ai_mesh = aiscene->mMeshes[meshIndex];

            uint32_t          material_id = ai_mesh->mMaterialIndex;

            glm::mat4 transform = glm::mat4(1.0f);

            scene->m_primInfos.push_back(
                PrimInfo({ .mesh_id = meshIndex, .material_id = material_id, .transform = transform })
                //PrimInfo({
                //    .mesh_id = meshIndex - 1, 
                //    .material_id = material_id,
                //    .transform = transform })
            );
        }
    }

    // 递归处理每个子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], aiscene, scene);
    }
}

void SceneLoader::processMeshs(const aiScene* aiscene, Scene* scene)
{
    auto numMeshes = aiscene->mNumMeshes;
    //auto _mesh = std::make_shared<Mesh>();
    scene->SetNumMeshes(numMeshes);
    //scene->SetNumMeshes(numMeshes - 1);

    for (auto i = 0; i < numMeshes; i++) {
        //if (i == 0) continue;

        auto mesh = aiscene->mMeshes[i];
        auto _mesh = std::make_shared<Mesh>();

        const int maxPos = 10000000;
        _mesh->m_box.pMax = glm::vec3(-maxPos, -maxPos, -maxPos);
        _mesh->m_box.pMin = glm::vec3(maxPos, maxPos, maxPos);

        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];

            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                _mesh->indices.push_back(face.mIndices[k]);
            }
        }

        // 遍历网格的所有顶点
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            Vertex vertex;
            vertex.position = glm::vec3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);
            if (mesh->mNormals) {
                vertex.normal = glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
            }
            if (mesh->mTangents && mesh->mBitangents) {
                vertex.tangent = glm::vec3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
                vertex.bitangent = glm::vec3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
            }
            //vertex.bitangent = glm::vec3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
            if (mesh->mTextureCoords[0]) {
                vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
            }
            else {
                vertex.texcoord = glm::vec2(0.0f, 0.0f);
            }

            _mesh->vertices.push_back(vertex);

            _mesh->m_box.pMin.x = std::min(_mesh->m_box.pMin.x, vertex.position.x);
            _mesh->m_box.pMin.y = std::min(_mesh->m_box.pMin.y, vertex.position.y);
            _mesh->m_box.pMin.z = std::min(_mesh->m_box.pMin.z, vertex.position.z);

            _mesh->m_box.pMax.x = std::max(_mesh->m_box.pMax.x, vertex.position.x);
            _mesh->m_box.pMax.y = std::max(_mesh->m_box.pMax.y, vertex.position.y);
            _mesh->m_box.pMax.z = std::max(_mesh->m_box.pMax.z, vertex.position.z);

            //_mesh->m_box.pMin = glm::min(_mesh->m_box.pMin, vertex.position);
            //_mesh->m_box.pMax = glm::min(_mesh->m_box.pMax, vertex.position);
        }

        //_mesh->matId = mesh->mMaterialIndex;

        scene->SetMesh(i, _mesh);
        //scene->SetMesh(i-1, _mesh);
    }

}


void SceneLoader::loadTexture(std::string path)
{

}

void SceneLoader::processTextures(const aiScene* aiscene)
{
    for (auto i = 0; i < aiscene->mNumTextures; i++) {
        spdlog::info("texture.name: " + std::string(aiscene->mTextures[i]->mFilename.C_Str()));
    }
}

void SceneLoader::processMaterials(const aiScene* aiscene, Scene* scene)
{
    spdlog::info("currentScenePath:{0}", m_currentScenePath);
    fs::path currentSceneRootPath = fs::path(m_currentScenePath).parent_path();

    for (auto i = 0; i < aiscene->mNumMaterials; i++) {
        //spdlog::info("material.name: " + std::string(aiscene->mMaterials[i]->GetName().C_Str()));

        aiMaterial* aimaterial = aiscene->mMaterials[i];
        aiString texturePath;

        std::shared_ptr<PhongMaterial> mat = std::make_shared<PhongMaterial>();

        if (aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            //spdlog::info("Diffuse texture:{0}", texturePath.C_Str());

            MImage<unsigned char> diffuseImage;
            std::shared_ptr<MVulkanTexture> texture = nullptr; //std::make_shared<MVulkanTexture>();

            std::string diffusePath = (currentSceneRootPath / texturePath.C_Str()).string();

            if (!Singleton<TextureManager>::instance().ExistTexture(diffusePath)) {
                if (diffuseImage.Load(diffusePath)) {
                    std::vector<MImage<unsigned char>*> images(1);
                    images[0] = &diffuseImage;
                    texture = std::make_shared<MVulkanTexture>();
                    //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                    Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
                }
                else
                {
	                spdlog::error("fail to load diffuse texture: {0}", diffusePath);
                }

                Singleton<TextureManager>::instance().Put(diffusePath, texture);
            }
            mat->diffuseTexture = diffusePath;
        }
        else {
            mat->diffuseTexture = "";
            //if (aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, mat->diffuseColor) != AI_SUCCESS) {
            //aiColor4D
            if (aimaterial->Get(AI_MATKEY_BASE_COLOR, mat->diffuseColor) != AI_SUCCESS) {
                mat->diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            //mat->diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
        }

        if (aimaterial->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS) {
            //spdlog::info("Specular texture:{0}", texturePath.C_Str());
        }
        else {
            mat->specularTexture = "";
            if (aimaterial->Get(AI_MATKEY_COLOR_SPECULAR, mat->specularColor) != AI_SUCCESS) {
                mat->specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
            }
        }

        if (aimaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
            //spdlog::info("Normal texture:{0}", texturePath.C_Str());

            MImage<unsigned char> normalImage;
            std::shared_ptr<MVulkanTexture> texture = nullptr;// std::make_shared<MVulkanTexture>();

            std::string normalPath = (currentSceneRootPath / texturePath.C_Str()).string();

            if (!Singleton<TextureManager>::instance().ExistTexture(normalPath)) {
                if (normalImage.Load(normalPath)) {
                    std::vector<MImage<unsigned char>*> images(1);
                    images[0] = &normalImage;
                    texture = std::make_shared<MVulkanTexture>();
                    //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                    Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
                }
                else
                {
                    spdlog::error("fail to load normal texture: {0}", normalPath);
                }

                Singleton<TextureManager>::instance().Put(normalPath, texture);
            }
            mat->normalMap = normalPath;
        }

        if (aimaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS) {
            //spdlog::info("roughness texture:{0}", texturePath.C_Str());

            MImage<unsigned char> roughnessImage;
            std::shared_ptr<MVulkanTexture> texture = nullptr; //= std::make_shared<MVulkanTexture>();

            std::string roughnessPath = (currentSceneRootPath / texturePath.C_Str()).string();

            if (!Singleton<TextureManager>::instance().ExistTexture(roughnessPath)) {
                if (roughnessImage.Load(roughnessPath)) {
                    texture = std::make_shared<MVulkanTexture>();
                    std::vector<MImage<unsigned char>*> images(1);
                    images[0] = &roughnessImage;
                    //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                    Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
                }
                else
                {
                    spdlog::error("fail to load roughness texture: {0}", roughnessPath);
                }

                Singleton<TextureManager>::instance().Put(roughnessPath, texture);
            }
            mat->metallicAndRoughnessTexture = roughnessPath;

        }
        else {
            mat->metallicAndRoughnessTexture = "";
        }

        if (aimaterial->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
            //spdlog::info("metalness texture:{0}", texturePath.C_Str());
        }

        if (aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat->roughness)) {
            //spdlog::info("AI_MATKEY_ROUGHNESS_FACTOR:{0}", mat->roughness);
        }

        if (aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, mat->metallic)) {
            //spdlog::info("AI_MATKEY_METALLIC_FACTOR:{0}", mat->metallic);
        }

        scene->AddMaterial(mat);
    }
}
