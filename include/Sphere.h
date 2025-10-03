#pragma once

#include "Shape.h"
#include <vector>
#include <glm/glm.hpp>

class Sphere : public Shape {
public:
    float radius = 0;
    Sphere(float radius, int subDivisions, glm::vec3 center = glm::vec3(0.0f));
    std::vector<float> getVertices() const;
    bool intersect(Ray) const;

private:
    std::vector<float> vertices;
};
