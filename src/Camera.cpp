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
    return cam.pos;
}

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
        cam.pos = ghostQuad.pos + glm::vec3(0.0f, 0.0f, 1.0f);
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


