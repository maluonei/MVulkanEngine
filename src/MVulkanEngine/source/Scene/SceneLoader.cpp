#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Managers/TextureManager.hpp"
#include <spdlog/spdlog.h>

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
            auto* const ai_mesh = aiscene->mMeshes[meshIndex];

            uint32_t          material_id = ai_mesh->mMaterialIndex;

            glm::mat4 transform = glm::mat4(1.0f);

            scene->m_primInfos.push_back(
                PrimInfo({ .mesh_id = meshIndex, .material_id = material_id, .transform = transform })
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

    for (auto i = 0; i < numMeshes; i++) {
        auto mesh = aiscene->mMeshes[i];
        auto _mesh = std::make_shared<Mesh>();

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
            vertex.normal = glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
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
        }

        //_mesh->matId = mesh->mMaterialIndex;

        scene->SetMesh(i, _mesh);
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
        spdlog::info("material.name: " + std::string(aiscene->mMaterials[i]->GetName().C_Str()));

        aiMaterial* aimaterial = aiscene->mMaterials[i];
        aiString texturePath;

        std::shared_ptr<PhongMaterial> mat = std::make_shared<PhongMaterial>();

        if (aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            spdlog::info("Diffuse texture:{0}", texturePath.C_Str());

            MImage<unsigned char> diffuseImage;
            std::shared_ptr<MVulkanTexture> texture = std::make_shared<MVulkanTexture>();

            std::string diffusePath = (currentSceneRootPath / texturePath.C_Str()).string();

            if (!Singleton<TextureManager>::instance().ExistTexture(diffusePath)) {
                if (diffuseImage.Load(diffusePath)) {
                    std::vector<MImage<unsigned char>*> images(1);
                    images[0] = &diffuseImage;
                    //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                    Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
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
            spdlog::info("Specular texture:{0}", texturePath.C_Str());
        }
        else {
            mat->specularTexture = "";
            if (aimaterial->Get(AI_MATKEY_COLOR_SPECULAR, mat->specularColor) != AI_SUCCESS) {
                mat->specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
            }
        }

        if (aimaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
            spdlog::info("Normal texture:{0}", texturePath.C_Str());

            MImage<unsigned char> normalImage;
            std::shared_ptr<MVulkanTexture> texture = std::make_shared<MVulkanTexture>();

            std::string normalPath = (currentSceneRootPath / texturePath.C_Str()).string();

            if (!Singleton<TextureManager>::instance().ExistTexture(normalPath)) {
                if (normalImage.Load(normalPath)) {
                    std::vector<MImage<unsigned char>*> images(1);
                    images[0] = &normalImage;
                    //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                    Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
                }

                Singleton<TextureManager>::instance().Put(normalPath, texture);
            }
            mat->normalMap = normalPath;
        }

        if (aimaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS) {
            spdlog::info("roughness texture:{0}", texturePath.C_Str());

            MImage<unsigned char> roughnessImage;
            std::shared_ptr<MVulkanTexture> texture = std::make_shared<MVulkanTexture>();

            std::string roughnessPath = (currentSceneRootPath / texturePath.C_Str()).string();

            if (!Singleton<TextureManager>::instance().ExistTexture(roughnessPath)) {
                if (roughnessImage.Load(roughnessPath)) {
                    std::vector<MImage<unsigned char>*> images(1);
                    images[0] = &roughnessImage;
                    //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                    Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
                }

                Singleton<TextureManager>::instance().Put(roughnessPath, texture);
            }
            mat->metallicAndRoughnessTexture = roughnessPath;

        }
        else {
            mat->metallicAndRoughnessTexture = "";
        }

        if (aimaterial->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
            spdlog::info("metalness texture:{0}", texturePath.C_Str());
        }

        if (aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat->roughness)) {
            spdlog::info("AI_MATKEY_ROUGHNESS_FACTOR:{0}", mat->roughness);
        }

        if (aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, mat->metallic)) {
            spdlog::info("AI_MATKEY_METALLIC_FACTOR:{0}", mat->metallic);
        }

        scene->AddMaterial(mat);
    }
}
