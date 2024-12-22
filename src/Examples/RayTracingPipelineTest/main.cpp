#include "test.hpp"

int main() {
	RayTracingPipelineTest test;
	test.Init();
	test.Run();
	test.Clean();
}