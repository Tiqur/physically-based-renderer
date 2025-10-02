#include "Transform.h"

glm::vec3 Transform::forward() const {
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(dir);
}

glm::vec3 Transform::right() const {
    return glm::normalize(glm::cross(forward(), worldUp()));
}

glm::vec3 Transform::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

glm::mat4 Transform::viewMatrix() const {
    return glm::lookAt(position, position + forward(), up());
}

glm::mat4 Transform::modelMatrix() const {
    return glm::inverse(viewMatrix());
}

glm::vec3 Transform::worldUp() {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}
