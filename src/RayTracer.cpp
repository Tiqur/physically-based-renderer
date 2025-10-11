#include "RayTracer.h"
#include <chrono>
#include <iostream>

RayTracer::RayTracer(int numRays, int maxSteps) : N(numRays), maxSteps(maxSteps) {
	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);

	ray_colors.setZero();
	ray_steps.setConstant(maxSteps);

	computeChunks();
}

// If user changes window size
void RayTracer::resize(int numRays) {
	N = numRays;
	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);

	ray_colors.setZero();
	ray_steps.setConstant(maxSteps);

	// We need to do this again since window may have more pixels/rays
	computeChunks();
}

void RayTracer::computeChunks() {
	chunks.clear();
	int raysPerThread = N / NUM_THREADS;
	int remainder = N % NUM_THREADS;

	int start = 0;
	for (int i = 0; i < NUM_THREADS; ++i) {
		int chunkSize = raysPerThread + (i < remainder ? 1 : 0);
		chunks.push_back({start, start + chunkSize});
		start += chunkSize;
	}
}

void RayTracer::initializeRays(Renderer& r) {

	Camera& cam = r.getCamera();
	Transform savedCamTransform = cam.getSavedCamTransform();
	glm::vec3 origin = savedCamTransform.position;
	Eigen::Vector3f cameraOrigin(origin.x, origin.y, origin.z);

	auto screenWidth = r.getWidth();
	auto screenHeight = r.getHeight();

	// Must have in case user resizes window
	// Also not in glfw resize callback due to performance
	int requiredRays = screenWidth * screenHeight;
	if (screenWidth * screenHeight != N) {
		resize(requiredRays);
	}

	auto plane = cam.getImagePlane();
	auto quadTopLeft = plane.topLeft();
	float quadWorldWidth = plane.worldSpaceWidth();

	// Offset each pixel to ensure ray is centered
	float pixelWidth = quadWorldWidth / (float)screenWidth;
	// float pixelHeight = quadWorldHeight / (float)screenHeight();

	// std::cout << screenWidth << std::endl;
	// std::cout << screenHeight << std::endl;
	//  exit(0);

	// Iterate for each pixel in image
	for (int x = 0; x < screenWidth; x++) {

		// Pixel offset right
		glm::vec3 offsetRight = plane.transform.right() * (pixelWidth * x);

		for (int y = 0; y < screenHeight; y++) {

			// Pixel offset down
			glm::vec3 offsetDown = plane.transform.up() * (pixelWidth * y);
			glm::vec3 posOnImagePlane = quadTopLeft + offsetRight - offsetDown;

			// Convert to Eigen
			Eigen::Vector3f posEigen(posOnImagePlane.x, posOnImagePlane.y, posOnImagePlane.z);

			// Set ray properties
			int index = x + y * screenWidth;
			ray_origins.col(index) = posEigen;
			ray_directions.col(index) = (posEigen - cameraOrigin).normalized();

			// TODO: I'm guessing for this we can either add another value to associate pixel with ray, or use chunks of N length?
			// Max dir can go in any direction for Monte Carlo based anti aliasing:
			// let dirToCenterOfPixel = <a, b, c>
			//
			//   { (offset, a, b, c) ∈ ℝ⁴ |
			//   a - (pw/2) < offset < a + (pw/2),
			//   b - (pw/2) < offset < b + (pw/2),
			//   c - (pw/2) < offset < c + (pw/2) }
		}
	}

	ray_colors.setZero();
}

void RayTracer::traceAllAsync() {
	// We don't want trace if it's already tracing
	if (isTracing())
		return;

	tracing = true;

	// Start in own separate thread so we can see it real-time
	std::thread([this]() {
		std::vector<std::thread> threads;

		for (int i = 0; i < NUM_THREADS; ++i) {
			threads.emplace_back(&RayTracer::traceChunk, this, i);
		}

		for (auto& t : threads) {
			t.join();
		}

		tracing = false;
	}).detach();
}

void RayTracer::traceChunk(int chunkIndex) {
	const ThreadChunk& chunk = chunks[chunkIndex];

	// Process rays until all are done
	bool anyActive = true;
	while (anyActive) {
		anyActive = false;

		for (int i = chunk.start; i < chunk.end; ++i) {

			// Fake delay so I can debug
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));

			if (ray_steps(0, i) < 1)
				continue;

			anyActive = true;
			ray_steps(0, i)--;

			// Update debug ray colors
			int color = (chunkIndex % 2 == 0) ? 100 : 255;
			ray_colors(0, i) = color;
			ray_colors(1, i) = color;
			ray_colors(2, i) = color;

			// TODO: Intersect with all objects in scene
		}
	}
}

void RayTracer::traceStep() {
	// std::vector<std::thread> threads;

	// for (int i = 0; i < NUM_THREADS; ++i) {
	//	threads.emplace_back([this, i]() {
	//		const ThreadChunk& chunk = chunks[i];
	//		for (int j = chunk.start; j < chunk.end; ++j) {
	//			if (ray_steps(0, j) < 1)
	//				continue;

	//			ray_steps(0, j)--;

	//			// TODO: Intersect with all objects
	//		}
	//	});
	//}

	// for (auto& t : threads) {
	//	t.join();
	// }
}
