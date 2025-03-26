#include "JsonLoader.hpp"
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

#include "Scene/Scene.hpp"
#include "Scene/ddgi.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"


glm::vec3 StrToVec3(std::string str) {
    std::istringstream iss(str);
    std::string token;
    std::vector<float> values;

    while (std::getline(iss, token, ',')) {
        // 去除前后空格
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        values.push_back(std::stof(token));
    }

    if (values.size() == 3) {
        return glm::vec3(values[0], values[1], values[2]);
    }

    return glm::vec3(0.f);
}

JsonFileLoader::JsonFileLoader(std::string filename)
{
	std::ifstream file(filename);

    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filename);
        //return 1;
    }

    file >> j;
}

std::shared_ptr<Camera> JsonFileLoader::GetCamera()
{
    //std::shared_ptr<Camera> camera = std::make_shared<Camera>();

    if (j.contains("Camera")) {
        for (const auto& cameraJson : j["Camera"]) {
            //auto cameraJson = j["Camera"][0];

            std::string posStr = cameraJson["position"];
            std::string dirStr = cameraJson["direction"];
            glm::vec3 pos = StrToVec3(posStr);
            glm::vec3 dir = glm::normalize(StrToVec3(dirStr));
            float fov = std::stof(std::string(cameraJson["fov"]));
            float nearPlane = std::stof(std::string(cameraJson["nearPlane"]));
            float farPlane = std::stof(std::string(cameraJson["farPlane"]));
            float aspectRatio = std::stof(std::string(cameraJson["aspectRatio"]));

            //auto res = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
            //float aspectRatio = res.width / res.height;

            std::shared_ptr<Camera> camera = std::make_shared<Camera>(pos, dir, fov, aspectRatio, nearPlane, farPlane);
            return camera;
        }
    }
    else {
        spdlog::error("no camera in json file");
    }

    return nullptr;
}

std::string JsonFileLoader::GetScenePath()
{
    if (j.contains("Scene")) {
        for (const auto& sceneJson : j["Scene"]) {
            return sceneJson["path"];
        }
    }
    else {
        spdlog::error("no scene in json file");
    }

    return "";
}

std::shared_ptr<Light> JsonFileLoader::GetLights()
{
    if (j.contains("Light")) {
        for (const auto& lightJson : j["Light"]) {
            //auto lightJson = j["Light"][0];

            std::string colorStr = lightJson["color"];
            std::string dirStr = lightJson["direction"];
            glm::vec3 color = StrToVec3(colorStr);
            glm::vec3 dir = glm::normalize(StrToVec3(dirStr));
            float intensity = std::stof(std::string(lightJson["intensity"]));

            std::shared_ptr<Light> light = std::make_shared<DirectionalLight>(dir, color, intensity);
            return light;
            //break;
        }
    }
    else {
        spdlog::error("no light in json file");
    }

    return nullptr;
}

glm::vec3 JsonFileLoader::GetDDGIProbeDim()
{
    if (j.contains("DDGIVolume")) {
        for (const auto& probeJson : j["DDGIVolume"]) {
            //auto probeJson = j["DDGIVolume"][0];

            std::string probeDimStr = probeJson["probeDim"];
            glm::ivec3 probeDim = StrToVec3(probeDimStr);

            return probeDim;
        }
    }
    else {
        spdlog::error("no DDGIVolume in json file");
    }

    return glm::ivec3(1, 1, 1);
}
