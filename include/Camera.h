#pragma once
#include "ImagePlane.h"
#include "Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
  public:
	struct MovementSettings {
		float rotationSpeed{5.0f};
		float moveSpeed{0.5f};
	};

	struct ProjectionSettings {
		float fov{45.0f};
		float nearPlane{0.1f};
		float farPlane{100.0f};
	};

  private:
	Transform cam;
	ImagePlane ghostQuad;
	Transform savedCam;

	MovementSettings movement;
	ProjectionSettings projection;

	bool ghostMode{false};

  public:
	Camera();

	// Getters
	float getFov() const;
	float getNearPlane() const;
	float getFarPlane() const;
	float getMoveSpeed() const;

	glm::vec3 getCamPos() const;
	float getCamYaw() const;
	float getCamPitch() const;
	void updateImagePlane(float width, float height);
	void setGhostQuadTransform(const Transform& newTransform);

	Transform getCamTransform() const;
	Transform getSavedCamTransform() const;

	glm::vec3 getGhostPos() const;
	float getGhostYaw() const;
	float getGhostPitch() const;

	bool getGhostMode() const;
	const ImagePlane getImagePlane() const;

	void toggleGhostMode();

	// Setters
	void setCamPos(glm::vec3);
	void setCamYaw(float);
	void setCamPitch(float);
	void setFov(float newFov);
	void setMoveSpeed(float newSpeed);
};
