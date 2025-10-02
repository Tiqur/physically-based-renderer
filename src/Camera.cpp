#include "Camera.h"

Camera::Camera() = default;

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
    return cam.position;
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
    return ghostQuad.transform.position;
}

float Camera::getGhostYaw() const {
    return ghostQuad.transform.yaw;
}

float Camera::getGhostPitch() const {
    return ghostQuad.transform.pitch;
}

bool Camera::getGhostMode() const {
    return ghostMode;
}

void Camera::updateImagePlane(float width, float height) {
    ghostQuad.width = width;
    ghostQuad.height = height;
}

const ImagePlane Camera::getImagePlane() const {
    return ghostQuad;
}

void Camera::setFov(float newFov) {
    if (newFov < 10.0f) newFov = 10.0f;
    if (newFov > 120.0f) newFov = 120.0f;
    projection.fov = newFov;
}

void Camera::setMoveSpeed(float newSpeed) {
    movement.moveSpeed = newSpeed;
}

void Camera::setGhostQuadTransform(const Transform& newTransform) {
    ghostQuad.transform = newTransform;
}

void Camera::toggleGhostMode() {
    if (!ghostMode) {
        setGhostQuadTransform(cam);
        savedCam = cam;

        glm::vec3 forward = cam.forward();
        float distance = 2.0f;
        cam.position = ghostQuad.transform.position - (forward * distance);
    } else {
        cam = ghostQuad.transform;
    }
    ghostMode = !ghostMode;
}

void Camera::setCamPos(glm::vec3 newPos) {
    cam.position = newPos;
    if (!ghostMode) {
      setGhostQuadTransform(cam);
    }
}

void Camera::setCamYaw(float newYaw) {
    cam.yaw = newYaw;
}

void Camera::setCamPitch(float newPitch) {
    cam.pitch = newPitch;
}

Transform Camera::getCamTransform() const {
  return cam;
}
