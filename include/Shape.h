#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Ray.h"

class Shape {
protected:
  std::vector<float> vertices;

public:
    glm::vec3 position{0.0f, 0.0f, 0.0f};

    virtual ~Shape() = default;

    virtual std::vector<float> getVertices() const = 0;

    virtual bool intersect(Ray) const;

    glm::mat4 getModelMatrix() const;
};

