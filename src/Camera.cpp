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

void Camera::updateImagePlane(float screenWidth, float screenHeight) {
	float aspectRatio = screenWidth / screenHeight;
	float planeHeight = 2.0f * projection.nearPlane * tan(glm::radians(getFov() / 2.0f));
	float planeWidth = planeHeight * aspectRatio;

	ghostQuad.width = planeWidth;
	ghostQuad.height = planeHeight;
}

const ImagePlane Camera::getImagePlane() const {
	return ghostQuad;
}

void Camera::setFov(float newFov) {
	if (newFov < 10.0f)
		newFov = 10.0f;
	if (newFov > 120.0f)
		newFov = 120.0f;
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

		savedCam = cam;

		Transform ghostTransform = savedCam;
		ghostTransform.position = savedCam.position + savedCam.forward() * projection.nearPlane;
		setGhostQuadTransform(ghostTransform);

		float pullBackDistance = 0.05f;
		cam.position = savedCam.position - savedCam.forward() * pullBackDistance;
	} else {
		cam = savedCam;
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

Transform Camera::getSavedCamTransform() const {
	return savedCam;
}
