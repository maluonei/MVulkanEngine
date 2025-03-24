#ifndef RANDOMGENERATOR_HPP
#define RANDOMGENERATOR_HPP


#include <random>
#include "Singleton.hpp"

class RandomGenerator : public Singleton<RandomGenerator> {
public:

	float GetRandomFloat();


private:
    virtual void InitSingleton();

	std::random_device rd;  // 硬件随机数种子
    std::mt19937 gen; // 伪随机数生成器（Mersenne Twister）
    std::uniform_int_distribution<int> dist; // 均匀分布 [1, 100]
};



#endif // GLOBALCONFIG_HPP