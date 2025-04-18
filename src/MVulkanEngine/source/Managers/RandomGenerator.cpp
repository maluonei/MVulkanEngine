#include "Managers/RandomGenerator.hpp"

float RandomGenerator::GetRandomFloat(){
    float value = dist(gen);
    return value / 1000.0f;
}

glm::vec3 RandomGenerator::GetRandomUnitVector()
{
    float phi = GetRandomFloat() * pi;
    float theta = GetRandomFloat() * 2.0f * pi;
    float x = sin(theta) * cos(phi);
    float y = sin(theta) * sin(phi);
    float z = cos(theta);

    return glm::normalize(glm::vec3(x, y, z));
}

RandomGenerator::RandomGenerator()
{
    gen = std::mt19937(rd());
    dist = std::uniform_int_distribution<int>(0, 999);
}

//void RandomGenerator::InitSingleton(){
//    gen =  std::mt19937(rd());
//    dist = std::uniform_int_distribution<int>(0, 999); 
//}