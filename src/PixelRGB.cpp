#include "PixelRGB.h"

void PixelRGB::setValues(float r, float g, float b) {
	data = glm::vec3(r, g, b);
}

glm::vec3 PixelRGB::getValues() const {
	return data;
}

uint32_t PixelRGB::toInt() const {
	uint8_t r = static_cast<uint8_t>(glm::clamp(data.r, 0.0f, 1.0f) * 255.0f);
	uint8_t g = static_cast<uint8_t>(glm::clamp(data.g, 0.0f, 1.0f) * 255.0f);
	uint8_t b = static_cast<uint8_t>(glm::clamp(data.b, 0.0f, 1.0f) * 255.0f);
	uint8_t a = 255;
	return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}
