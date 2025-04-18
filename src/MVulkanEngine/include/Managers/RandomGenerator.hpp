#ifndef RANDOMGENERATOR_HPP
#define RANDOMGENERATOR_HPP


#include <random>
#include "Singleton.hpp"
#include <glm/glm.hpp>

class RandomGenerator : public Singleton<RandomGenerator> {
public:
	float GetRandomFloat();
    glm::vec3 GetRandomUnitVector();

protected:
    const float pi = 3.1415926536;

    friend class Singleton<RandomGenerator>; // 让 Singleton 访问构造函数
    RandomGenerator(); // 只能通过 Singleton::instance() 访问

    //virtual void InitSingleton() override;

	std::random_device rd;  // 硬件随机数种子
    std::mt19937 gen; // 伪随机数生成器（Mersenne Twister）
    std::uniform_int_distribution<int> dist; // 均匀分布 [1, 100]
};



#endif // GLOBALCONFIG_HPP