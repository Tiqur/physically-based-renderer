#include "Renderer.h"
#include "RayTracer.h"
#include "Shader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <Eigen/Dense>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Static member initialization
Renderer* Renderer::instance = nullptr;

static const char* rasterVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
      gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

static const char* rasterFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 uColor;

    void main() {
      FragColor = uColor;
    }
)";

static const char* quadVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec2 TexCoord;

    void main() {
      TexCoord = aTexCoord;
      gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
    }
)";

static const char* quadFragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D uTexture;

    void main() {
      FragColor = texture(uTexture, TexCoord);
    }
)";

Renderer::Renderer(int width, int height)
    : window(nullptr), screenWidth(width), screenHeight(height), cam(), rayVBO(nullptr), rayVAO(nullptr), textureID(0), isDragging(false), lastMouseX(0.0), lastMouseY(0.0), mouseSensitivity(0.1f) {

	// Create and set instance
	instance = this;
}

Renderer::~Renderer() {
	cleanupRays();
	cleanupShapes();

	if (textureID) {
		glDeleteTextures(1, &textureID);
	}

	if (window) {
		glfwDestroyWindow(window);
	}

	instance = nullptr;
}

bool Renderer::initialize() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window
	window = glfwCreateWindow(screenWidth, screenHeight, "Ray Tracer", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return false;
	}

	// Setup viewport
	glViewport(0, 0, screenWidth, screenHeight);

	// Setup OpenGL state
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(2.0f);

	// Initialize shaders and buffers
	initializeShaders();
	initializeBuffers();
	initializeTexture();

	// Initialize image plane texture
	initializeImagePlaneTexture();

	return true;
}

void Renderer::initializeImagePlaneTexture() {
	glGenTextures(1, &imagePlaneTexture);
	glBindTexture(GL_TEXTURE_2D, imagePlaneTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	std::vector<uint32_t> initData(screenWidth * screenHeight, 0x22FFFFFF);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, initData.data());
}

void Renderer::renderFrustrum() {
	if (!cam.getGhostMode())
		return;

	// TODO: We shouldn't calculate all this every frame
	Transform savedCamTransform = cam.getSavedCamTransform();
	glm::vec3 origin = savedCamTransform.position;
	const ImagePlane& plane = cam.getImagePlane();

	// Get vertices of the ImagePlane
	glm::vec3 nearCorners[4] = {
	    plane.topLeft(),
	    plane.topRight(),
	    plane.bottomRight(),
	    plane.bottomLeft()};

	// Calculate far plane corners
	float farPlane = cam.getFarPlane();
	glm::vec3 farCorners[4];
	for (int i = 0; i < 4; i++) {
		glm::vec3 dir = glm::normalize(nearCorners[i] - origin);
		farCorners[i] = origin + dir * farPlane;
	}

	std::vector<float> verts;
	verts.reserve(72); // 12 lines * 2 vertices * 3 floats

	auto addLine = [&](const glm::vec3& a, const glm::vec3& b) {
		verts.insert(verts.end(), {a.x, a.y, a.z, b.x, b.y, b.z});
	};

	// Near plane rectangle
	for (int i = 0; i < 4; i++) {
		addLine(nearCorners[i], nearCorners[(i + 1) % 4]);
	}

	// Connecting lines
	for (int i = 0; i < 4; i++) {
		addLine(nearCorners[i], farCorners[i]);
	}

	// Far plane rectangle
	for (int i = 0; i < 4; i++) {
		addLine(farCorners[i], farCorners[(i + 1) % 4]);
	}

	VBO vbo(&verts);
	VAO vao;
	vao.bind();
	vbo.bind();
	vao.setAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	rasterProgram->use();
	glm::mat4 view = cam.getCamTransform().viewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(cam.getFov()),
	    (float)screenWidth / (float)screenHeight,
	    cam.getNearPlane(),
	    cam.getFarPlane());

	setupRasterUniforms(glm::mat4(1.0f), view, projection, glm::vec4(0.0f, 1.0f, 1.0f, 0.4f));
	glDrawArrays(GL_LINES, 0, 24);
}

// void Renderer::castRays(std::vector<Ray>& rays, std::vector<Shape*>& worldObjects) {
//
//	// Do each chunk on separate thread
//	// int chunks = 4;
//	// Eigen::Matrix<float, 3, 10> ray_origins;
//	// Eigen::Matrix<float, 3, 10> ray_directions;
//
//	std::vector<PixelRGB> pixels(screenWidth * screenHeight);
//
//	// Initialize all pixels to background color
//	for (int i = 0; i < screenWidth * screenHeight; i++) {
//		pixels[i] = PixelRGB(glm::vec3(0.0f, 0.0f, 0.0f));
//	}
//
//	// Check for intersections
//	size_t rayIndex = 0;
//	for (int x = 0; x < screenWidth; x++) {
//		for (int y = 0; y < screenHeight; y++) {
//
//			if (rayIndex >= rays.size())
//				break;
//
//			bool hitAnything = false;
//			for (Shape* obj : worldObjects) {
//				if (obj->intersect(rays[rayIndex])) {
//					hitAnything = true;
//					break;
//				}
//			}
//
//			// Simple skybox
//			float rayY = rays[rayIndex].direction.y;
//			float t = 0.8f * rayY + 1.0f;
//			glm::vec3 sky_color = glm::vec3(1.0f, 1.0f, 1.0f) * (1.0f - t) + glm::vec3(0.5f, 0.7f, 1.0f) * t;
//
//			glm::vec4 color = hitAnything ? glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(sky_color, 1.0f);
//
//			// Convert 2D coordinates to 1D index
//			int pixelIndex = y * screenWidth + x;
//			pixels[pixelIndex] = PixelRGB(color);
//
//			rayIndex++;
//		}
//	}
//
//	updateTexture(pixels);
// }

void Renderer::initializeShaders() {
	std::string rasterVert(rasterVertexShaderSource);
	std::string rasterFrag(rasterFragmentShaderSource);
	Shader rasterVertexShader(&rasterVert, GL_VERTEX_SHADER);
	Shader rasterFragmentShader(&rasterFrag, GL_FRAGMENT_SHADER);
	rasterProgram = std::make_unique<ShaderProgram>(
	    std::move(rasterVertexShader),
	    std::move(rasterFragmentShader));

	std::string quadVert(quadVertexShaderSource);
	std::string quadFrag(quadFragmentShaderSource);
	Shader quadVertexShader(&quadVert, GL_VERTEX_SHADER);
	Shader quadFragmentShader(&quadFrag, GL_FRAGMENT_SHADER);
	quadProgram = std::make_unique<ShaderProgram>(
	    std::move(quadVertexShader),
	    std::move(quadFragmentShader));
}

void Renderer::initializeBuffers() {
	std::vector<float> vertices = {
	    -1.0f,
	    -1.0f,
	    0.0f,
	    1.0f,
	    1.0f,
	    -1.0f,
	    1.0f,
	    1.0f,
	    1.0f,
	    1.0f,
	    1.0f,
	    0.0f,

	    -1.0f,
	    -1.0f,
	    0.0f,
	    1.0f,
	    1.0f,
	    1.0f,
	    1.0f,
	    0.0f,
	    -1.0f,
	    1.0f,
	    0.0f,
	    0.0f};

	quadVBO = std::make_unique<VBO>(&vertices);
	quadVAO = std::make_unique<VAO>();

	// Position attribute (location 0)
	quadVAO->setAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture coordinate attribute (location 1)
	quadVAO->setAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void Renderer::initializeTexture() {
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

void Renderer::setupCallbacks(GLFWwindow* win) {
	glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);
	glfwSetScrollCallback(win, scrollCallback);
	glfwSetCursorPosCallback(win, cursorPositionCallback);
	glfwSetMouseButtonCallback(win, mouseButtonCallback);
}

void Renderer::beginFrame() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void Renderer::setupRasterUniforms(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec4& color) {
	GLuint modelLoc = glGetUniformLocation(rasterProgram->id(), "model");
	GLuint viewLoc = glGetUniformLocation(rasterProgram->id(), "view");
	GLuint projectionLoc = glGetUniformLocation(rasterProgram->id(), "projection");
	GLint colorLoc = glGetUniformLocation(rasterProgram->id(), "uColor");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform4f(colorLoc, color.r, color.g, color.b, color.a);
}

// TODO: Use Instancing
void Renderer::renderRays(const RayTracer& tracer, int rayStep) {
	if (!rayVAO || !rayVBO) {
		return;
	}

	size_t numRays = (size_t)tracer.getNumRays();
	// const Eigen::Matrix<float, 3, Eigen::Dynamic>& origins = tracer.getRayOrigins();
	// const Eigen::Matrix<float, 3, Eigen::Dynamic>& directions = tracer.getRayDirections();

	if (numRays == 0)
		return;

	rasterProgram->use();

	glm::mat4 view = cam.getCamTransform().viewMatrix();
	glm::mat4 projection = glm::perspective(
	    glm::radians(cam.getFov()),
	    (float)screenWidth / (float)screenHeight,
	    cam.getNearPlane(),
	    cam.getFarPlane());
	glm::mat4 identity = glm::mat4(1.0f);

	int max_steps = tracer.getMaxSteps();
	Eigen::Array<int, 1, Eigen::Dynamic> ray_steps = tracer.getRaySteps();

	for (size_t i = 0; i < numRays; ++i) {
		// Don't render rays unless they've been traced at least once
		if (ray_steps[i] == max_steps)
			continue;

		size_t row = i / screenWidth;
		size_t col = i % screenWidth;
		if (row % rayStep != 0 || col % rayStep != 0)
			continue;

		// TODO
		bool hitAnything = false;
		glm::vec4 color = hitAnything ? glm::vec4(0.0f, 1.0f, 0.0f, 0.1f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.04f);

		rayVAO->bind();
		setupRasterUniforms(identity, view, projection, color);
		glDrawArrays(GL_LINES, i * 2, 2);
	}
}

void Renderer::renderShapes(const std::vector<Shape*>& shapes) {
	if (shapes.empty() || shapeVAOs.empty())
		return;

	rasterProgram->use();

	glm::mat4 view = cam.getCamTransform().viewMatrix();
	glm::mat4 projection = glm::perspective(
	    glm::radians(cam.getFov()),
	    (float)screenWidth / (float)screenHeight,
	    cam.getNearPlane(),
	    cam.getFarPlane());

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (size_t i = 0; i < shapes.size(); i++) {
		glm::mat4 model = shapes[i]->getModelMatrix();
		glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);

		setupRasterUniforms(model, view, projection, color);
		shapeVAOs[i]->bind();
		glDrawArrays(GL_TRIANGLES, 0, shapes[i]->getVertices().size() / 3);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::renderImagePlane() {
	quadProgram->use();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	quadVAO->bind();

	GLuint modelLoc = glGetUniformLocation(quadProgram->id(), "model");
	GLuint viewLoc = glGetUniformLocation(quadProgram->id(), "view");
	GLuint projectionLoc = glGetUniformLocation(quadProgram->id(), "projection");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imagePlaneTexture);
	glUniform1i(glGetUniformLocation(quadProgram->id(), "uTexture"), 0);

	if (cam.getGhostMode()) {
		cam.updateImagePlane((float)screenWidth, (float)screenHeight);

		const ImagePlane& plane = cam.getImagePlane();
		glm::mat4 model = plane.modelMatrix();
		glm::mat4 view = cam.getCamTransform().viewMatrix();
		glm::mat4 projection = glm::perspective(
		    glm::radians(cam.getFov()),
		    (float)screenWidth / (float)screenHeight,
		    cam.getNearPlane(),
		    cam.getFarPlane());

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	} else {
		// Full screen quad
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 orthoProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 hudView = glm::mat4(1.0f);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(orthoProjection));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(hudView));
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

// TODO: Move to RayTracer class
void Renderer::setupRayBuffers(const RayTracer& tracer) {
	cleanupRays(); // clear old buffers

	size_t numRays = tracer.getNumRays();
	float rayLength = 128.0f;
	std::vector<float> vertices;
	vertices.reserve(numRays * 6);

	const auto& origins = tracer.getRayOrigins();
	const auto& directions = tracer.getRayDirections();

	for (size_t i = 0; i < numRays; i++) {
		Eigen::Vector3f origin = origins.col(i);
		Eigen::Vector3f direction = directions.col(i);

		Eigen::Vector3f endpoint = origin + direction * rayLength;

		// Add line vertices (origin -> endpoint)
		vertices.push_back(origin.x());
		vertices.push_back(origin.y());
		vertices.push_back(origin.z());

		vertices.push_back(endpoint.x());
		vertices.push_back(endpoint.y());
		vertices.push_back(endpoint.z());
	}

	// Upload all vertices at once
	rayVBO = new VBO(&vertices);
	rayVAO = new VAO();

	rayVAO->bind();
	rayVBO->bind();
	rayVAO->setAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

void Renderer::setupShapeBuffers(const std::vector<Shape*>& shapes) {
	cleanupShapes();

	for (Shape* shape : shapes) {
		std::vector<float> shapeVerts = shape->getVertices();
		VBO* shapeVBO = new VBO(&shapeVerts);
		VAO* shapeVAO = new VAO();

		shapeVAO->bind();
		shapeVBO->bind();
		shapeVAO->setAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		shapeVBOs.push_back(shapeVBO);
		shapeVAOs.push_back(shapeVAO);
	}
}

void Renderer::cleanupRays() {
	// delete rayVBO;
	// delete rayVAO;
}

void Renderer::cleanupShapes() {
	for (VBO* vbo : shapeVBOs)
		delete vbo;
	for (VAO* vao : shapeVAOs)
		delete vao;
	shapeVBOs.clear();
	shapeVAOs.clear();
}

void Renderer::updateTexture(const Eigen::Matrix<int, 3, Eigen::Dynamic>& colors_matrix) {
	std::vector<uint32_t> uint_rgb_data;
	uint_rgb_data.reserve(colors_matrix.cols());

	for (int i = 0; i < colors_matrix.cols(); i++) {
		Eigen::Vector3i color = colors_matrix.col(i);

		uint8_t r = static_cast<uint8_t>(color[0]);
		uint8_t g = static_cast<uint8_t>(color[1]);
		uint8_t b = static_cast<uint8_t>(color[2]);
		uint8_t a = 255;
		uint32_t pixel = (r << 0) | (g << 8) | (b << 16) | (a << 24);

		uint_rgb_data.push_back(pixel);
	}

	glBindTexture(GL_TEXTURE_2D, imagePlaneTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, uint_rgb_data.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::setDimensions(int width, int height) {
	screenWidth = width;
	screenHeight = height;
	glViewport(0, 0, width, height);
}

void Renderer::processInput(float deltaTime) {
	ImGuiIO& io = ImGui::GetIO();

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (io.WantCaptureKeyboard)
		return;

	glm::vec3 forward = cam.getCamTransform().forward();
	glm::vec3 right = cam.getCamTransform().right();
	glm::vec3 up = cam.getCamTransform().up();

	float velocity = cam.getMoveSpeed() * deltaTime * 10.0f;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cam.setCamPos(cam.getCamPos() + forward * velocity);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cam.setCamPos(cam.getCamPos() - forward * velocity);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cam.setCamPos(cam.getCamPos() - right * velocity);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cam.setCamPos(cam.getCamPos() + right * velocity);
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		cam.setCamPos(cam.getCamPos() + up * velocity);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
	    glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
		cam.setCamPos(cam.getCamPos() - up * velocity);
	}
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	(void)window;
	if (instance) {
		instance->setDimensions(width, height);

		// Resize tracer
		// TODO: Doing all this inside resize callback makes it lag when resizing window
		if (instance->tracerPtr) {
			instance->tracerPtr->resize(width * height);
			instance->tracerPtr->initializeRays(*instance);
			instance->setupRayBuffers(*instance->tracerPtr);
		}
	}
}

void Renderer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse || !instance)
		return;

	float currentFov = instance->cam.getFov();
	currentFov -= (float)yoffset * 2.0f;
	instance->cam.setFov(currentFov);
}

void Renderer::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

	if (!instance)
		return;

	ImGuiIO& io = ImGui::GetIO();
	bool shouldDrag = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse;

	if (shouldDrag) {
		if (!instance->isDragging) {
			instance->isDragging = true;
			instance->lastMouseX = xpos;
			instance->lastMouseY = ypos;
		} else {
			double xoffset = xpos - instance->lastMouseX;
			double yoffset = instance->lastMouseY - ypos;

			instance->lastMouseX = xpos;
			instance->lastMouseY = ypos;

			instance->cam.setCamYaw(instance->cam.getCamYaw() + xoffset * instance->mouseSensitivity);
			instance->cam.setCamPitch(instance->cam.getCamPitch() + yoffset * instance->mouseSensitivity);

			if (instance->cam.getCamPitch() > 89.0f)
				instance->cam.setCamPitch(89.0f);
			if (instance->cam.getCamPitch() < -89.0f)
				instance->cam.setCamPitch(-89.0f);
		}
	} else {
		instance->isDragging = false;
	}
}
