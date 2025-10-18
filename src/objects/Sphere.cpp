#include "Sphere.h"
#include "Ray.h"
#include "Material.h"
#include <vector>
#include <map>
#include <algorithm>

static glm::vec3 getMidpoint(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::normalize(v1 + v2);
}

Sphere::Sphere(float radius, int subdivisions, glm::vec3 center, Material mat) {
    this->material = mat;
    this->radius = radius;
    this->position = center;

    // Golden Ratio 
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    std::vector<glm::vec3> initialVertices = {
        glm::normalize(glm::vec3(-1,  t,  0)), glm::normalize(glm::vec3( 1,  t,  0)), glm::normalize(glm::vec3(-1, -t,  0)), glm::normalize(glm::vec3( 1, -t,  0)),
        glm::normalize(glm::vec3( 0, -1,  t)), glm::normalize(glm::vec3( 0,  1,  t)), glm::normalize(glm::vec3( 0, -1, -t)), glm::normalize(glm::vec3( 0,  1, -t)),
        glm::normalize(glm::vec3( t,  0, -1)), glm::normalize(glm::vec3( t,  0,  1)), glm::normalize(glm::vec3(-t,  0, -1)), glm::normalize(glm::vec3(-t,  0,  1))
    };

    std::vector<glm::ivec3> faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    // Subdivide Recursively
    std::map<std::pair<int, int>, int> midpointCache;

    for (int i = 0; i < subdivisions; ++i) {
        std::vector<glm::ivec3> newFaces;
        for (const auto& face : faces) {
            int v1_idx = face.x;
            int v2_idx = face.y;
            int v3_idx = face.z;

            // Calculate midpoints
            auto getMidpointIndex = [&](int idx1, int idx2) {
                std::pair<int, int> edge = std::minmax(idx1, idx2);
                if (midpointCache.find(edge) == midpointCache.end()) {
                    glm::vec3 mid = getMidpoint(initialVertices[idx1], initialVertices[idx2]);
                    initialVertices.push_back(mid);
                    midpointCache[edge] = initialVertices.size() - 1;
                }
                return midpointCache[edge];
            };

            int m1_idx = getMidpointIndex(v1_idx, v2_idx);
            int m2_idx = getMidpointIndex(v2_idx, v3_idx);
            int m3_idx = getMidpointIndex(v3_idx, v1_idx);

            // Create new faces
            newFaces.push_back({v1_idx, m1_idx, m3_idx});
            newFaces.push_back({v2_idx, m2_idx, m1_idx});
            newFaces.push_back({v3_idx, m3_idx, m2_idx});
            newFaces.push_back({m1_idx, m2_idx, m3_idx});
        }
        faces = newFaces;
        midpointCache.clear();
    }

    // Add to vertex buffer
    for (const auto& face : faces) {
        glm::vec3 v1 = initialVertices[face.x] * radius;
        glm::vec3 v2 = initialVertices[face.y] * radius;
        glm::vec3 v3 = initialVertices[face.z] * radius;

        vertices.push_back(v1.x); vertices.push_back(v1.y); vertices.push_back(v1.z);
        vertices.push_back(v2.x); vertices.push_back(v2.y); vertices.push_back(v2.z);
        vertices.push_back(v3.x); vertices.push_back(v3.y); vertices.push_back(v3.z);
    }
}

std::vector<float> Sphere::getVertices() const {
    return vertices;
}
