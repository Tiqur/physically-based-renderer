#include "Cube.h"
#include "Ray.h"
#include "RayTracer.h"
#include "Renderer.h"
#include "Shape.h"
#include "Sphere.h"
#include "Square.h"
#include "Triangle.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <Eigen/Dense>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#define MEASURE_LOGS false
bool renderToImagePlane = false;

// TODO: Remove magic numbers
RayTracer tracer(800 * 600, 16);
Renderer renderer(800, 600);

// Scene data
std::vector<Shape*> worldObjects;

// Renderer settings
// static int threadCount = 1;

// For performance/debugging
static int rayStep = 16;

// Delta time
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// FOR DEBUGGING / BENCHMARKS
// TODO: Move to own file/class
void measure(const std::string& name, auto func) {
	auto start = std::chrono::high_resolution_clock::now();
	func();
	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	if (MEASURE_LOGS)
		std::cout << name << " took " << duration.count() << " ms\n";
}

void cleanupScene() {
	for (Shape* shape : worldObjects) {
		delete shape;
	}
	worldObjects.clear();
}

void setupScene() {
	Sphere* sphere;
	sphere = new Sphere(0.4f, 4, glm::vec3(0.0f, 0.0f, -5.0f));
	worldObjects.push_back(sphere);
	sphere = new Sphere(1.0f, 4, glm::vec3(0.0f, 1.0f, -10.0f));
	worldObjects.push_back(sphere);
	sphere = new Sphere(4.0f, 4, glm::vec3(0.0f, 2.0f, -15.0f));
	worldObjects.push_back(sphere);

	// "Floor"
	float radius = (float)(2 << 12);
	sphere = new Sphere(radius, 6, glm::vec3(0.0f, -radius - 1.0f, -5.0f));
	worldObjects.push_back(sphere);
}

void renderUI(Renderer& renderer) {
	// For Debugging
	ImGui::ShowMetricsWindow();

	ImGui::Begin("Scene Settings");

	ImGui::Button("Load/Select Scene");

	if (ImGui::Button("Reset")) {
		renderToImagePlane = false;
		renderer.cleanupRays();
		renderer.resetImagePlaneView();
	}

	// ImGui::SliderInt("Thread Count", &threadCount, 1, 16);
	ImGui::SliderInt("RayStep", &rayStep, 1, 128);

	if (ImGui::Button("Step")) {
		tracer.traceStep();
	}
	if (ImGui::Button("Render")) {
		renderer.cleanupRays();
		tracer.initializeRays(renderer);
		renderer.setupRayBuffers(tracer);
		tracer.traceAllAsync(worldObjects);
		renderToImagePlane = true;
	}

	ImGui::End();

	ImGui::Begin("Camera");

	Camera& cam = renderer.getCamera();
	if (ImGui::Button(cam.getGhostMode() ? "Disable Ghost" : "Enable Ghost")) {
		cam.toggleGhostMode();
	}

	ImGui::End();
}

int main() {
	std::cout << "Initializing ImGui..." << std::endl;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	if (!renderer.initialize()) {
		std::cerr << "Failed to initialize renderer!" << std::endl;
		return -1;
	}

	// We need this for resize callback
	renderer.setTracer(&tracer);

	std::cout << "Initializing ImGui backends..." << std::endl;
	if (!ImGui_ImplGlfw_InitForOpenGL(renderer.getWindow(), true)) {
		std::cerr << "Failed to initialize ImGui GLFW backend!" << std::endl;
		return -1;
	}

	if (!ImGui_ImplOpenGL3_Init("#version 330")) {
		std::cerr << "Failed to initialize ImGui OpenGL backend!" << std::endl;
		return -1;
	}

	// Setup callbacks
	renderer.setupCallbacks(renderer.getWindow());

	// Setup scene
	setupScene();
	renderer.setupShapeBuffers(worldObjects);

	std::cout << renderer.getWidth() << std::endl;
	std::cout << renderer.getHeight() << std::endl;

	// Main loop
	while (!glfwWindowShouldClose(renderer.getWindow())) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Setup
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		renderer.beginFrame();

		renderer.updateTexture(tracer.getRayColors());
		renderer.renderRays(tracer, rayStep);
		renderer.renderShapes(worldObjects);
		renderer.renderFrustrum();

		if (renderToImagePlane)
			renderer.renderImagePlane();

		// Render UI
		renderUI(renderer);

		// Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Process input
		renderer.processInput(deltaTime);
		renderer.endFrame();
	}

	cleanupScene();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();

	return 0;
}
