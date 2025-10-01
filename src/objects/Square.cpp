#include "Square.h"

Square::Square(float size, glm::vec3 center) {
    float halfSize = size / 2.0f;
    glm::vec3 v0 = glm::vec3(center.x - halfSize, center.y - halfSize, center.z);
    glm::vec3 v1 = glm::vec3(center.x + halfSize, center.y - halfSize, center.z);
    glm::vec3 v2 = glm::vec3(center.x + halfSize, center.y + halfSize, center.z);
    glm::vec3 v3 = glm::vec3(center.x - halfSize, center.y + halfSize, center.z);

    // First triangle
    vertices.push_back(v0.x); vertices.push_back(v0.y); vertices.push_back(v0.z);
    vertices.push_back(v1.x); vertices.push_back(v1.y); vertices.push_back(v1.z);
    vertices.push_back(v2.x); vertices.push_back(v2.y); vertices.push_back(v2.z);

    // Second triangle
    vertices.push_back(v0.x); vertices.push_back(v0.y); vertices.push_back(v0.z);
    vertices.push_back(v2.x); vertices.push_back(v2.y); vertices.push_back(v2.z);
    vertices.push_back(v3.x); vertices.push_back(v3.y); vertices.push_back(v3.z);
}

std::vector<float> Square::getVertices() const {
    return vertices;
}
