#pragma once
#include "Shape.h"

class Triangle : public Shape {
public:
    glm::vec3 v0, v1, v2;

    Triangle(glm::vec3 vertex0 = glm::vec3(-0.5f, -0.5f, 0.0f),
             glm::vec3 vertex1 = glm::vec3(0.5f, -0.5f, 0.0f),
             glm::vec3 vertex2 = glm::vec3(0.0f, 0.5f, 0.0f));

    std::vector<float> getVertices() const override;
};

