#include "Managers/RandomGenerator.hpp"

float RandomGenerator::GetRandomFloat(){
    float value = dist(gen);
    return value / 1000.0f;
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