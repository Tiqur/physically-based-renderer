#include "Square.h"

Square::Square(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) {
    glm::vec3 p3 = p1 + (p2 - p0);

    // First triangle
    vertices.push_back(p0.x); vertices.push_back(p0.y); vertices.push_back(p0.z);
    vertices.push_back(p1.x); vertices.push_back(p1.y); vertices.push_back(p1.z);
    vertices.push_back(p2.x); vertices.push_back(p2.y); vertices.push_back(p2.z);

    // Second triangle
    vertices.push_back(p1.x); vertices.push_back(p1.y); vertices.push_back(p1.z);
    vertices.push_back(p3.x); vertices.push_back(p3.y); vertices.push_back(p3.z);
    vertices.push_back(p2.x); vertices.push_back(p2.y); vertices.push_back(p2.z);
}

std::vector<float> Square::getVertices() const {
    return vertices;
}
