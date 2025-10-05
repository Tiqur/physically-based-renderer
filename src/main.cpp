#include "Cube.h"
#include "Ray.h"
#include "Renderer.h"
#include "Shape.h"
#include "Sphere.h"
#include "Square.h"
#include "Triangle.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// Scene data
std::vector<Ray> rays;
std::vector<Shape*> worldObjects;

// Renderer settings
static int threadCount = 1;

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
	std::cout << name << " took " << duration.count() << " ms\n";
}

void cleanupRays() {
	// for (Ray* ray : rays) {
	//	delete ray;
	// }
	rays.clear();
}

void cleanupScene() {
	for (Shape* shape : worldObjects) {
		delete shape;
	}
	worldObjects.clear();
}

void setupScene() {
	// Cube* cube = new Cube();
	// cube->position = glm::vec3(0.0f, 0.0f, -10.0f);
	// worldObjects.push_back(cube);

	Sphere* sphere = new Sphere(1.0f, 4, glm::vec3(0.0f, 0.0f, -5.0f));
	worldObjects.push_back(sphere);

	// Square* square = new Square();
	// square->position = glm::vec3(0.0f, 0.0f, -5.0f);
	// worldObjects.push_back(square);

	// for (int i = 0; i < 32; i++) {
	//	Triangle* triangle = new Triangle();
	//	triangle->position = glm::vec3(0.0f, 0.0f, (float)(-i) * 0.1);
	//	worldObjects.push_back(triangle);
	// }
}

void renderUI(Renderer& renderer) {
	// For Debugging
	ImGui::ShowMetricsWindow();

	ImGui::Begin("Scene Settings");

	ImGui::Button("Load/Select Scene");
	ImGui::Button("Start");
	ImGui::Button("Stop");

	if (ImGui::Button("Reset")) {
		cleanupRays();
		renderer.cleanupRays();
	}

	ImGui::SliderInt("Thread Count", &threadCount, 1, 16);
	ImGui::SliderInt("RayStep", &rayStep, 1, 128);

	if (ImGui::Button("Render")) {

		measure("Cleanup Rays", [&] {
			cleanupRays();
			renderer.cleanupRays();
		});

		measure("Generate Rays", [&] {
			renderer.generateRays(rays);
		});

		measure("Setup Buffers", [&] {
			renderer.setupRayBuffers(rays);
		});

		measure("Cast Rays", [&] {
			renderer.castRays(rays, worldObjects);
		});

		// std::thread t1(&Renderer::castRays, &renderer, std::ref(rays), std::ref(worldObjects));
		// std::cout << "Ray Count: " << rays.size() << std::endl;
	}

	ImGui::End();

	ImGui::Begin("Camera");

	Camera& cam = renderer.getCamera();
	if (ImGui::Button(cam.getGhostMode() ? "Disable Ghost" : "Enable Ghost")) {
		cam.toggleGhostMode();
		cleanupRays();
		renderer.cleanupRays();
	}

	ImGui::End();
}

int main() {
	std::cout << "Initializing ImGui..." << std::endl;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	Renderer renderer(800, 600);
	if (!renderer.initialize()) {
		std::cerr << "Failed to initialize renderer!" << std::endl;
		return -1;
	}

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

		// Render scene
		measure("Render Rays", [&] {
			renderer.renderRays(rays, worldObjects, rayStep);
		});

		measure("Render Shapes", [&] {
			renderer.renderShapes(worldObjects);
		});
		measure("Render Frustrum", [&] {
			renderer.renderFrustrum();
		});
		measure("Render ImagePlane", [&] {
			renderer.renderImagePlane();
		});

		// Render UI
		renderUI(renderer);

		// Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Process input
		renderer.processInput(deltaTime);
		renderer.endFrame();
	}

	cleanupRays();
	cleanupScene();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();

	return 0;
}
