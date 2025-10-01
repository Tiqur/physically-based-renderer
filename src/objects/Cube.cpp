#include "Cube.h"
#include <glm/glm.hpp>
#include <vector>

Cube::Cube(float size, glm::vec3 center) {
    float halfSize = size / 2.0f;
    glm::vec3 min = center - glm::vec3(halfSize);
    glm::vec3 max = center + glm::vec3(halfSize);

    // Front face
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(max.z);
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(max.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(max.z);

    // Back face
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(min.z);

    // Left face
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(max.z);
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(max.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(min.z);

    // Right face
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(max.z);

    // Top face
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(min.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(max.z);
    vertices.push_back(max.x); vertices.push_back(max.y); vertices.push_back(min.z);
    vertices.push_back(min.x); vertices.push_back(max.y); vertices.push_back(min.z);

    // Bottom face
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(min.z);
    vertices.push_back(max.x); vertices.push_back(min.y); vertices.push_back(max.z);
    vertices.push_back(min.x); vertices.push_back(min.y); vertices.push_back(max.z);
}

std::vector<float> Cube::getVertices() const {
    return vertices;
}
