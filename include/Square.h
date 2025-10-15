#pragma once
#include "Shape.h"

class Square : public Shape {
  public:
	Square(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2);

	std::vector<float> getVertices() const override;
};
