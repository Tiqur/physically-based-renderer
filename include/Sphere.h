#pragma once

#include "Shape.h"
#include <glm/glm.hpp>
#include <vector>

class Sphere : public Shape {
  public:
	float radius = 0;
	Sphere(float radius, int subDivisions, glm::vec3 center = glm::vec3(0.0f), Material mat = Material::NORMAL);
	std::vector<float> getVertices() const;
	bool intersect(Ray) const;

  private:
	std::vector<float> vertices;
};
