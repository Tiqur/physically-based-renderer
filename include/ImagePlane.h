#pragma once

#include "Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ImagePlane {
  private:
	float width = 1.0f;
	float height = 1.0f;

  public:
	Transform transform;
	float getWidth() const { return width; }
	float getHeight() const { return height; }
	void setWidth(float w) { width = w; }
	void setHeight(float h) { height = h; }

	glm::mat4 modelMatrix() const;

	// Get world-space corner pos
	glm::vec3 topLeft() const;
	glm::vec3 topRight() const;
	glm::vec3 bottomLeft() const;
	glm::vec3 bottomRight() const;

	// Get world-space dims
	float worldSpaceWidth() const;
	float worldSpaceHeight() const;
};
