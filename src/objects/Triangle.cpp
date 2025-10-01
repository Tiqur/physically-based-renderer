#include "Triangle.h"

Triangle::Triangle(glm::vec3 vertex0,
                   glm::vec3 vertex1,
                   glm::vec3 vertex2)
    : v0(vertex0), v1(vertex1), v2(vertex2) {}

std::vector<float> Triangle::getVertices() const {
    return {
        v0.x, v0.y, v0.z,
        v1.x, v1.y, v1.z,
        v2.x, v2.y, v2.z
    };
}

