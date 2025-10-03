#include "Shape.h"
#include "Ray.h"

glm::mat4 Shape::getModelMatrix() const {
    return glm::translate(glm::mat4(1.0f), position);
}

bool Shape::intersect(Ray ray) const {
    (void)ray;
    return false;
}


