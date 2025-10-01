#include "Camera.h"

// Constructor
Camera::Camera() = default;

// Getters
float Camera::getFov() const {
    return projection.fov;
}

float Camera::getNearPlane() const {
    return projection.nearPlane;
}

float Camera::getFarPlane() const {
    return projection.farPlane;
}

glm::vec3 Camera::getCamPos() const {
    return cam.pos; }
float Camera::getCamYaw() const {
    return cam.yaw;
}

float Camera::getCamPitch() const {
    return cam.pitch;
}

float Camera::getMoveSpeed() const {
    return movement.moveSpeed;
}

glm::vec3 Camera::getGhostPos() const {
    return ghostQuad.pos;
}

float Camera::getGhostYaw() const {
    return ghostQuad.yaw;
}

float Camera::getGhostPitch() const {
    return ghostQuad.pitch;
}

bool Camera::getGhostMode() const {
    return ghostMode;
}

void Camera::setFov(float newFov) {
    if (newFov < 10.0f) newFov = 10.0f;
    if (newFov > 120.0f) newFov = 120.0f;
    projection.fov = newFov;
}

void Camera::setMoveSpeed(float newSpeed) {
    movement.moveSpeed = newSpeed;
}

// Toggle Ghost Mode
void Camera::toggleGhostMode() {
    if (!ghostMode) {
        ghostQuad = cam;
        savedCam = cam;

        // Calculate distance offset using normal vector
        glm::vec3 forward = getCamForward();

        float distance = 2.0f;

        cam.pos = ghostQuad.pos - (forward * distance);
    } else {
        cam = ghostQuad;
    }
    ghostMode = !ghostMode;
}

void Camera::setCamPos(glm::vec3 newPos) {
  cam.pos = newPos;
}

void Camera::setCamYaw(float newYaw) {
  cam.yaw = newYaw;
}

void Camera::setCamPitch(float newPitch) {
  cam.pitch = newPitch;
}

glm::vec3 Camera::getCamForward() {
    glm::vec3 forward;
    forward.x = cos(glm::radians(getGhostYaw())) * cos(glm::radians(getGhostPitch()));
    forward.y = sin(glm::radians(getGhostPitch()));
    forward.z = sin(glm::radians(getGhostYaw())) * cos(glm::radians(getGhostPitch()));
    forward = glm::normalize(forward);
    return forward;
}


glm::vec3 Camera::getGhostForward() {
    glm::vec3 forward;
    forward.x = cos(glm::radians(getCamYaw())) * cos(glm::radians(getCamPitch()));
    forward.y = sin(glm::radians(getCamPitch()));
    forward.z = sin(glm::radians(getCamYaw())) * cos(glm::radians(getCamPitch()));
    forward = glm::normalize(forward);
    return forward;
}
