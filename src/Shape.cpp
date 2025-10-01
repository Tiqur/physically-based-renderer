#include "Shape.h"

glm::mat4 Shape::getModelMatrix() const {
    return glm::translate(glm::mat4(1.0f), position);
}

