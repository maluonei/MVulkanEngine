#include "ssr.hpp"

#include<glm/gtc/matrix_transform.hpp>
#include<glm/ext/matrix_clip_space.hpp>
#include<glm/gtc/type_ptr.hpp>

int main() {

	glm::vec3 position = { 4, 5, 6 };
	glm::vec3 direction = { -0.612372,-0.5,-0.612372 };
	glm::vec3 target = { 3.38763,4.5,5.38763 };
	target = position + direction;
	glm::vec3 euler = { -30, 45, 0 };

	glm::vec4 new_direction = { 0, 0, -1, 1 };
	glm::mat4 rot_mat = glm::mat4(1.f);
	glm::mat4 rot_x = glm::rotate(glm::mat4(1.f), glm::radians(euler[0]), glm::vec3(1.f, 0.f, 0.f));
	glm::mat4 rot_y = glm::rotate(glm::mat4(1.f), glm::radians(euler[1]), glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 rot_z = glm::rotate(glm::mat4(1.f), glm::radians(euler[2]), glm::vec3(0.f, 0.f, 1.f));

	new_direction = rot_z * rot_y * rot_x * new_direction;
	std::cout << "new_direction:" << new_direction[0] << "," << new_direction[1] << "," << new_direction[2] << "," << new_direction[3] << std::endl;
	std::cout << "target:" << target[0] << "," << target[1] << "," << target[2] << std::endl;

	std::cout << "glm::radians(90.):" << glm::radians(90.) << std::endl;
	std::cout << "glm::radians(39.597755335771296):" << glm::radians(39.597755335771296) << std::endl;

	glm::mat4 la = glm::lookAt(position, position + glm::vec3(new_direction), glm::vec3(0.f, 1.f, 0.f));
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			std::cout << la[i][j] << ",";
		}
		std::cout << std::endl;
	}
	exit(0);

	SSR ssr;
	ssr.Init();
	ssr.Run();
	ssr.Clean();
}