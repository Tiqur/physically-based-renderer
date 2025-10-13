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

// TODO: Remove magic numbers
RayTracer tracer(800 * 600, 16);
Renderer renderer(800, 600);

// void testIntersections() {
//	int default_ray_steps = 8;
//	Eigen::Array<int, 1, 2> ray_steps;        // Negative ray step = inactive
//	ray_steps.setConstant(default_ray_steps); // Set all to default value
//
//	Eigen::Matrix<int, 3, 2> ray_colors; // RGB - Start at 0
//	ray_colors.col(0) << 0, 0, 0;
//	ray_colors.col(1) << 0, 0, 0;
//	std::cout << ray_colors << "\n---\n";
//
//	Eigen::Matrix<float, 3, 2> ray_origins;
//	ray_origins.col(0) << -1, -1, -1; // Should intersect
//	ray_origins.col(1) << 1, 1, 1;    // Shouldn't intersect
//	std::cout << ray_origins << "\n---\n";
//
//	Eigen::Matrix<float, 3, 2> ray_directions;
//	ray_directions.col(0) << 1, 1, 1;
//	ray_directions.col(1) << 1, 1, 1;
//	std::cout << ray_directions << "\n---\n";
//
//	Eigen::Vector3f sphere_center(0, 0, 0);
//	float sphere_radius = 1.0f;
//
//	for (int i = 0; i < ray_origins.cols(); i++) {
//		// Skip masked out rays
//		if (ray_steps(0, i) < 0)
//			continue;
//
//		ray_steps(0, i)--;
//
//		Eigen::Vector3f o = ray_origins.col(i);
//		Eigen::Vector3f d = ray_directions.col(i);
//		d.normalize();
//
//		Eigen::Vector3f oc = o - sphere_center;
//
//		float a = d.dot(d);
//		float b = 2 * d.dot(oc);
//		float c = oc.dot(oc) - sphere_radius * sphere_radius;
//
//		float discriminant = b * b - 4 * a * c;
//
//		if (discriminant < 0) {
//			std::cout << "Ray " << i << " misses the sphere\n";
//			// Mask out - NEGATIVE STEP MEANS INACTIVE
//			ray_steps(0, i) = -1;
//		} else {
//			float t1 = (-b - std::sqrt(discriminant)) / (2 * a);
//			float t2 = (-b + std::sqrt(discriminant)) / (2 * a);
//			float t = (t1 > 0) ? t1 : ((t2 > 0) ? t2 : -1);
//			if (t < 0) {
//				std::cout << "Ray " << i << " hits behind the origin\n";
//				// Mask out
//				ray_steps(0, i) = -1;
//			} else {
//				Eigen::Vector3f hit_point = o + t * d;
//				std::cout << "Ray " << i << " intersects at t=" << t << ", point=" << hit_point.transpose() << "\n";
//
//				// TODO: Replace these test values
//         // These will update depending on the material
//
//				// Update colors
//				ray_colors.col(i) << 255, 0, 0;
//
//				// Update origin
//				ray_origins.col(i) << ray_origins.col(i) + Eigen::Vector3f::Ones();
//
//				// Update directions
//				ray_directions.col(i) << (ray_directions.col(i) + Eigen::Vector3f::Ones());
//				ray_directions.col(i).normalize();
//
//				// Check for intersections again (loop)
//			}
//		}
//	}
//
//	// ray_origins + t*ray_directions
//
//	// int t = 10;
//	// std::cout << (((ray_origins + t * ray_directions) /*-sphere_pos*/).transpose()) * (((ray_origins + t * ray_directions) /*-sphere_pos*/)) << std::endl;
//
//	// Eigen::Matrix<float, 1, 3> sphere;
//	// sphere.row(0) << 1, 1, 1;
//	// std::cout << sphere << std::endl;
//	// std::cout << (sphere * sphere.transpose()) << std::endl;
//
//	exit(0);
// }

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
	// Cube* cube = new Cube();
	// cube->position = glm::vec3(0.0f, 0.0f, -10.0f);
	// worldObjects.push_back(cube);

	Sphere* sphere;
	sphere = new Sphere(0.8f, 4, glm::vec3(0.0f, 0.0f, -10.0f));
	worldObjects.push_back(sphere);
	sphere = new Sphere(1.0f, 4, glm::vec3(0.0f, 0.0f, -5.0f));
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

	if (ImGui::Button("Reset")) {
		renderer.cleanupRays();
	}

	// ImGui::SliderInt("Thread Count", &threadCount, 1, 16);
	ImGui::SliderInt("RayStep", &rayStep, 1, 128);

	if (ImGui::Button("Step")) {
		tracer.traceStep();
	}
	if (ImGui::Button("Render")) {
		// tracer.cleanupRays();
		tracer.initializeRays(renderer);
		renderer.setupRayBuffers(tracer);
		tracer.traceAllAsync(worldObjects);

		// renderer.castRays(rays, worldObjects);

		// measure("Cleanup Rays", [&] {
		//	cleanupRays();
		//	renderer.cleanupRays();
		// });

		// measure("Generate Rays", [&] {
		//	renderer.generateRays(rays);
		// });

		// measure("Setup Buffers", [&] {
		//	renderer.setupRayBuffers(rays);
		// });

		// measure("Cast Rays", [&] {
		//	renderer.castRays(rays, worldObjects);
		// });

		// std::thread t1(&Renderer::castRays, &renderer, std::ref(rays), std::ref(worldObjects));
		// std::cout << "Ray Count: " << rays.size() << std::endl;
	}

	ImGui::End();

	ImGui::Begin("Camera");

	Camera& cam = renderer.getCamera();
	if (ImGui::Button(cam.getGhostMode() ? "Disable Ghost" : "Enable Ghost")) {
		cam.toggleGhostMode();
		renderer.cleanupRays();
	}

	ImGui::End();
}

int main() {
	// testIntersections();
	std::cout << "Initializing ImGui..." << std::endl;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

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

		measure("Update Texture", [&] {
			if (tracer.isTracing())
				renderer.updateTexture(tracer.getRayColors());
		});

		// Render scene
		measure("Render Rays", [&] {
			renderer.renderRays(tracer, rayStep);
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

	cleanupScene();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();

	return 0;
}
