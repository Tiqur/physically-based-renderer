#pragma once
#include "Shape.h"

class Cube : public Shape {
  public:
	Cube(float size = 1.0f, glm::vec3 center = glm::vec3(0.0f));

	std::vector<float> getVertices() const override;
};
