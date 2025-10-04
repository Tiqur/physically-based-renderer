#pragma once
#include <glm/glm.hpp>

class PixelRGB {
  private:
	glm::vec3 data;

  public:
	PixelRGB() : data(0.0f) {}
	PixelRGB(float r, float g, float b) : data(r, g, b) {}
	PixelRGB(glm::vec3 d) : data(d) {}

	void setValues(float r, float g, float b);

	glm::vec3 getValues() const;

	uint32_t toInt() const;

	// Convert from packed int to glm::vec3
	static PixelRGB fromInt(uint32_t color) {
		float r = ((color >> 16) & 0xFF) / 255.0f;
		float g = ((color >> 8) & 0xFF) / 255.0f;
		float b = (color & 0xFF) / 255.0f;
		return PixelRGB(r, g, b);
	}
};
