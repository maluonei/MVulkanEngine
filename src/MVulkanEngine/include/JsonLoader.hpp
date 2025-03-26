#ifndef JSON_LOADER_HPP
#define JSON_LOADER_HPP

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <memory>

class Camera;
class Scene;
class Light;
class DDGIVolume;

using json = nlohmann::json;

class JsonFileLoader {
public:
	JsonFileLoader(std::string filename);

	std::shared_ptr<Camera> GetCamera();
	std::string GetScenePath();
	std::shared_ptr<Light> GetLights();
	glm::vec3 GetDDGIProbeDim();

private:
	json j;
};

#endif