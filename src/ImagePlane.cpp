#include "ImagePlane.h"

glm::mat4 ImagePlane::modelMatrix() const {
    return glm::scale(transform.modelMatrix(), glm::vec3(width, height, 1.0f));
}

glm::vec3 ImagePlane::topLeft() const {
    return transform.position 
        - transform.right() * (width / 2.0f) 
        + transform.up() * (height / 2.0f);
}

glm::vec3 ImagePlane::topRight() const {
    return transform.position 
        + transform.right() * (width / 2.0f) 
        + transform.up() * (height / 2.0f);
}

glm::vec3 ImagePlane::bottomLeft() const {
    return transform.position 
        - transform.right() * (width / 2.0f) 
        - transform.up() * (height / 2.0f);
}

glm::vec3 ImagePlane::bottomRight() const {
    return transform.position 
        + transform.right() * (width / 2.0f) 
        - transform.up() * (height / 2.0f);
}

float ImagePlane::worldSpaceWidth() const {
    return glm::length(topRight() - topLeft());
}

float ImagePlane::worldSpaceHeight() const {
    return glm::length(topLeft() - bottomLeft());
}

