#include "Managers/RandomGenerator.hpp"

float RandomGenerator::GetRandomFloat(){
    return dist(gen) / 1000.0f;
}

void RandomGenerator::InitSingleton(){
    gen =  std::mt19937(rd());
    dist = std::uniform_int_distribution<int>(0, 999); 
}