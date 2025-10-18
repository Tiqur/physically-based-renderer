#pragma once
#include "Material.h"
#include "Ray.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Shape {
  protected:
	std::vector<float> vertices;
	Material material;

  public:
	glm::vec3 position{0.0f, 0.0f, 0.0f};

	virtual ~Shape() = default;

	virtual std::vector<float> getVertices() const = 0;
	virtual Material getMaterial() const { return material; };

	glm::mat4 getModelMatrix() const;
};
