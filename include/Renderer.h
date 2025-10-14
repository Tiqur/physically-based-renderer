#pragma once

#include "Camera.h"
#include "Ray.h"
#include "RayTracer.h"
#include "ShaderProgram.h"
#include "Shape.h"
#include "VAO.h"
#include "VBO.h"
#include <Eigen/Dense>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class RayTracer;

class Renderer {
  public:
	Renderer(int width, int height);
	~Renderer();

	bool initialize();
	void setupCallbacks(GLFWwindow* window);
	void setTracer(RayTracer* tracer) { tracerPtr = tracer; }

	// Rendering
	void beginFrame();
	void endFrame();
	void renderRays(const RayTracer& tracer, int rayStep);
	void renderFrustrum();
	void renderShapes(const std::vector<Shape*>& shapes);
	void renderImagePlane();

	// Rays
	// void generateRays(std::vector<Ray>& rays);
	void castRays(std::vector<Ray>& rays, std::vector<Shape*>& worldObjects);
	void setupRayBuffers(const RayTracer& tracer);
	void cleanupRays();

	// Shapes
	void setupShapeBuffers(const std::vector<Shape*>& shapes);
	void cleanupShapes();

	// Textures
	GLuint imagePlaneTexture;
	void updateTexture(const Eigen::Matrix<int, 3, Eigen::Dynamic>& colors_matrix);
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
	RayTracer* tracerPtr = nullptr;

	// Shaders
	std::unique_ptr<ShaderProgram> rasterProgram;
	std::unique_ptr<ShaderProgram> quadProgram;

	// Quad Buffers
	std::unique_ptr<VBO> quadVBO;
	std::unique_ptr<VAO> quadVAO;

	// Ray Buffers
	VBO* rayVBO;
	VAO* rayVAO;

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
