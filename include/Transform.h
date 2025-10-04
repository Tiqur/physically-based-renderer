#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
  public:
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;

	glm::vec3 forward() const;
	glm::vec3 right() const;
	glm::vec3 up() const;

	// Convert world to local
	glm::mat4 viewMatrix() const;

	// Convert local to world
	glm::mat4 modelMatrix() const;

	// World space direction: up
	static glm::vec3 worldUp();
};
