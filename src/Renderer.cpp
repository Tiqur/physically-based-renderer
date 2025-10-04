#include "Renderer.h"
#include "Shader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
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
    : window(nullptr), screenWidth(width), screenHeight(height), cam(), textureID(0), isDragging(false), lastMouseX(0.0), lastMouseY(0.0), mouseSensitivity(0.1f) {

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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Testing:
	srand(static_cast<unsigned>(time(nullptr)));
	std::vector<unsigned char> pixels(screenWidth * screenHeight * 4);
	for (int y = 0; y < screenHeight; ++y) {
		for (int x = 0; x < screenWidth; ++x) {
			int i = (y * screenWidth + x) * 4;
			pixels[i + 0] = rand() % 256; // R
			pixels[i + 1] = rand() % 256; // G
			pixels[i + 2] = rand() % 256; // B
			pixels[i + 3] = 64;           // A
		}
	}

	// Update texture
	glBindTexture(GL_TEXTURE_2D, imagePlaneTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

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

void Renderer::renderRays(const std::vector<Ray*>& rays, const std::vector<Shape*>& worldObjects) {
	if (rays.empty() || rayVAOs.empty())
		return;

	rasterProgram->use();

	glm::mat4 view = cam.getCamTransform().viewMatrix();
	glm::mat4 projection = glm::perspective(
	    glm::radians(cam.getFov()),
	    (float)screenWidth / (float)screenHeight,
	    cam.getNearPlane(),
	    cam.getFarPlane());
	glm::mat4 identity = glm::mat4(1.0f);

	for (size_t i = 0; i < rays.size(); i++) {
		bool hitAnything = false;
		for (Shape* obj : worldObjects) {
			if (obj->intersect(*rays[i])) {
				hitAnything = true;
				break;
			}
		}

		glm::vec4 color = hitAnything ? glm::vec4(0.0f, 1.0f, 0.0f, 0.05f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.02f);

		setupRasterUniforms(identity, view, projection, color);
		rayVAOs[i]->bind();
		glDrawArrays(GL_LINES, 0, 2);
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

void Renderer::renderImagePlane(bool ghostMode) {
	quadProgram->use();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	quadVAO->bind();

	GLuint modelLoc = glGetUniformLocation(quadProgram->id(), "model");
	GLuint viewLoc = glGetUniformLocation(quadProgram->id(), "view");
	GLuint projectionLoc = glGetUniformLocation(quadProgram->id(), "projection");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imagePlaneTexture);
	glUniform1i(glGetUniformLocation(quadProgram->id(), "uTexture"), 0);

	if (ghostMode) {
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

void Renderer::generateRays(std::vector<Ray*>& rays) {
	Transform savedCamTransform = cam.getSavedCamTransform();
	glm::vec3 origin = savedCamTransform.position;

	// Offset each ray from top left point on ImagePlane for reference
	const ImagePlane& plane = cam.getImagePlane();
	glm::vec3 quadTopLeft = plane.topLeft();

	// Offset each pixel to ensure ray is centered
	float quadWorldWidth = plane.worldSpaceWidth();
	float pixelWidth = quadWorldWidth / (float)screenWidth;

	// Iterate for each pixel in image
	for (int x = 0; x <= screenWidth; x++) {
		if (x % 10 != 0)
			continue;

		// Pixel offset right
		glm::vec3 offsetRight = plane.transform.right() * (pixelWidth * x);

		for (int y = 0; y <= screenHeight; y++) {
			if (y % 10 != 0)
				continue;

			// Pixel offset down
			glm::vec3 offsetDown = plane.transform.up() * (pixelWidth * y);
			glm::vec3 posOnImagePlane = quadTopLeft + offsetRight - offsetDown;

			// Normalize each vector to get a field of unit vectors (each representing cast direction)
			glm::vec3 direction = glm::normalize(posOnImagePlane - origin);

			// Max dir can go in any direction for Monte Carlo based anti aliasing:
			// let dirToCenterOfPixel = <a, b, c>
			//
			//   { (offset, a, b, c) ∈ ℝ⁴ |
			//   a - (pw/2) < offset < a + (pw/2),
			//   b - (pw/2) < offset < b + (pw/2),
			//   c - (pw/2) < offset < c + (pw/2) }

			bool renderOnPlane = true;
			Ray* ray = new Ray(renderOnPlane ? posOnImagePlane : origin, direction);
			rays.push_back(ray);
		}
	}
}

void Renderer::setupRayBuffers(const std::vector<Ray*>& rays) {
	cleanupRays();

	float rayLength = 128.0f;
	for (Ray* ray : rays) {
		glm::vec3 p0 = ray->origin;
		glm::vec3 p1 = ray->origin + ray->direction * rayLength;

		std::vector<float> vertices = {
		    p0.x,
		    p0.y,
		    p0.z,
		    p1.x,
		    p1.y,
		    p1.z};

		VBO* rayVBO = new VBO(&vertices);
		VAO* rayVAO = new VAO();

		rayVAO->bind();
		rayVBO->bind();
		rayVAO->setAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		rayVBOs.push_back(rayVBO);
		rayVAOs.push_back(rayVAO);
	}
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
	for (VBO* vbo : rayVBOs)
		delete vbo;
	for (VAO* vao : rayVAOs)
		delete vao;
	rayVBOs.clear();
	rayVAOs.clear();
}

void Renderer::cleanupShapes() {
	for (VBO* vbo : shapeVBOs)
		delete vbo;
	for (VAO* vao : shapeVAOs)
		delete vao;
	shapeVBOs.clear();
	shapeVAOs.clear();
}

void Renderer::updateTexture(const std::vector<unsigned char>& pixels) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
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
