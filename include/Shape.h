#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Shape {
public:
    glm::vec3 position{0.0f, 0.0f, 0.0f};

    virtual ~Shape() = default;

    // Pure virtual function for vertex data
    virtual std::vector<float> getVertices() const = 0;

    // Transform to world space
    glm::mat4 getModelMatrix() const;
};

