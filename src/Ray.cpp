#include "Ray.h"
#include <glm/glm.hpp>

Ray::Ray(const glm::vec3& o, const glm::vec3& d)
    : origin(o), direction(glm::normalize(d)) {}

glm::vec3 Ray::at(float t) const {
    return origin + t * direction;
}

