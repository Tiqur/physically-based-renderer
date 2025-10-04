#pragma once

#include "Camera.h"
#include "Ray.h"
#include "ShaderProgram.h"
#include "Shape.h"
#include "VAO.h"
#include "VBO.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Renderer {
  public:
	Renderer(int width, int height);
	~Renderer();

	bool initialize();
	void setupCallbacks(GLFWwindow* window);

	// Rendering
	void beginFrame();
	void endFrame();
	void renderRays(const std::vector<Ray*>& rays, const std::vector<Shape*>& worldObjects);
	void renderShapes(const std::vector<Shape*>& shapes);
	void renderImagePlane(bool ghostMode);

	// Rays
	void generateRays(std::vector<Ray*>& rays);
	void setupRayBuffers(const std::vector<Ray*>& rays);
	void cleanupRays();

	// Shapes
	void setupShapeBuffers(const std::vector<Shape*>& shapes);
	void cleanupShapes();

	// Textures
	GLuint imagePlaneTexture;
	void updateTexture(const std::vector<unsigned char>& pixels);
	void initializeImagePlaneTexture();

	// Utility
	GLFWwindow* getWindow() const { return window; }
	Camera& getCamera() { return cam; }
	int getWidth() const { return screenWidth; }
	int getHeight() const { return screenHeight; }
	void setDimensions(int width, int height);
	void processInput(float deltaTime);

	// callbacks
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

  private:
	GLFWwindow* window;
	int screenWidth;
	int screenHeight;
	Camera cam;

	// Shaders
	std::unique_ptr<ShaderProgram> rasterProgram;
	std::unique_ptr<ShaderProgram> quadProgram;

	// Quad Buffers
	std::unique_ptr<VBO> quadVBO;
	std::unique_ptr<VAO> quadVAO;

	// Ray Buffers
	std::vector<VBO*> rayVBOs;
	std::vector<VAO*> rayVAOs;

	// Shape Buffers
	std::vector<VBO*> shapeVBOs;
	std::vector<VAO*> shapeVAOs;

	// Texture
	GLuint textureID;

	// Mouse state
	bool isDragging;
	double lastMouseX;
	double lastMouseY;
	float mouseSensitivity;

	// Helper methods
	void initializeShaders();
	void initializeBuffers();
	void initializeTexture();
	void setupRasterUniforms(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec4& color);
	static Renderer* instance;
};
