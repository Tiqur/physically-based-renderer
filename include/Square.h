#pragma once
#include "Shape.h"

class Square : public Shape {
public:
    Square(float size = 1.0f, glm::vec3 center = glm::vec3(0.0f));

    std::vector<float> getVertices() const override;
};
