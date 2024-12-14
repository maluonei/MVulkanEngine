#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Managers/TextureManager.hpp"

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

    // 通过指定路径加载模型
    const aiScene* aiscene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_GenNormals);

    // 检查加载是否成功
    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
        //std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        spdlog::error("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }

    processTextures(aiscene);
    processMaterials(aiscene, scene);

    processNode(aiscene->mRootNode, aiscene, scene);

    scene->GenerateIndirectDrawData();

    scene->CalculateBB();
}

void SceneLoader::processNode(const aiNode* node, const aiScene* aiscene, Scene* scene)
{
    spdlog::info("node.name: " + std::string(node->mName.C_Str()));
    spdlog::info("node.mNumChildren: " + std::to_string(node->mNumChildren));

    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = aiscene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, aiscene, scene);
    }

    // 递归处理每个子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], aiscene, scene);
    }
}

void SceneLoader::processMesh(const aiMesh* mesh, const aiScene* aiscene, Scene* scene)
{
    std::shared_ptr<Mesh> _mesh = std::make_shared<Mesh>();
    std::string meshName = mesh->mName.C_Str();
    if (meshName == "") {
        meshName = "mesh" + std::to_string(g_meshId);
        g_meshId++;
    }

    spdlog::info("mesh.name: " + meshName);
    spdlog::info("mesh.mNumVertices: " + std::to_string(mesh->mNumVertices));


    // 遍历网格的索引
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            _mesh->indices.push_back(face.mIndices[j]);
            //spdlog::info("indices:" + std::to_string(face.mIndices[j] + currentVertexNum));
        }
    }

    // 遍历网格的所有顶点
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) {
            vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else {
            vertex.texcoord = glm::vec2(0.0f, 0.0f);
        }
        _mesh->vertices.push_back(vertex);
    }

    _mesh->matId = mesh->mMaterialIndex;

    if (mesh->mNumVertices > 0) {
        scene->SetMesh(meshName, _mesh);
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
            std::cout << "Diffuse texture: " << texturePath.C_Str() << std::endl;
            
            MImage<unsigned char> diffuseImage;
            std::shared_ptr<MVulkanTexture> texture = std::make_shared<MVulkanTexture>();

            std::string diffusePath = (currentSceneRootPath / texturePath.C_Str()).string();

            if (diffuseImage.Load(diffusePath)) {
                std::vector<MImage<unsigned char>*> images(1);
                images[0] = &diffuseImage;
                //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
            }

            Singleton<TextureManager>::instance().Put(diffusePath, texture);
            mat->diffuseTexture = diffusePath;
        }
        else {
            mat->diffuseTexture = "";
            mat->diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
        }

        if (aimaterial->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS) {
            std::cout << "Specular texture: " << texturePath.C_Str() << std::endl;
        }

        if (aimaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
            std::cout << "Normal map: " << texturePath.C_Str() << std::endl;
        }

        if (aimaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS) {
            std::cout << "roughness map: " << texturePath.C_Str() << std::endl;

            MImage<unsigned char> roughnessImage;
            std::shared_ptr<MVulkanTexture> texture = std::make_shared<MVulkanTexture>();

            std::string roughnessPath = (currentSceneRootPath / texturePath.C_Str()).string();
            if (roughnessImage.Load(roughnessPath)) {
                std::vector<MImage<unsigned char>*> images(1);
                images[0] = &roughnessImage;
                //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
                Singleton<MVulkanEngine>::instance().CreateImage(texture, images, true);
            }

            Singleton<TextureManager>::instance().Put(roughnessPath, texture);
            mat->metallicAndRoughnessTexture = roughnessPath;
        }
        else {
            mat->metallicAndRoughnessTexture = "";
        }

        if (aimaterial->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
            std::cout << "metalness map: " << texturePath.C_Str() << std::endl;
        }

        if (aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat->roughness)) {
            std::cout << "AI_MATKEY_ROUGHNESS_FACTOR: " << mat->roughness << std::endl;
        }

        if (aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, mat->metallic)) {
            std::cout << "AI_MATKEY_METALLIC_FACTOR: " << mat->metallic << std::endl;
        }

        scene->AddMaterial(mat);
    }
}
