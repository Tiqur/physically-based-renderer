#include "RayTracer.h"
#include <chrono>
#include <iostream>
#include <limits>

RayTracer::RayTracer(int numRays, int maxSteps) : N(numRays), maxSteps(maxSteps) {
	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);
	t_distance.resize(1, N);

	computeChunks();
}

// If user changes window size
void RayTracer::resize(int numRays) {
	N = numRays;
	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);
	t_distance.resize(1, N);

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
	ray_colors.setZero();
	ray_steps.setConstant(maxSteps);
	t_distance.setConstant(std::numeric_limits<float>::infinity()); // We use infinity so that ANY object hit will be closer

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

void RayTracer::traceAllAsync(const std::vector<Shape*>& worldObjects) {
	// We don't want trace if it's already tracing
	if (isTracing())
		return;

	tracing = true;

	// Start in own separate thread so we can see it real-time
	std::thread([this, &worldObjects]() {
		std::vector<std::thread> threads;

		for (int i = 0; i < NUM_THREADS; ++i) {
			threads.emplace_back(&RayTracer::traceChunk, this, i, worldObjects);
		}

		for (auto& t : threads) {
			t.join();
		}

		tracing = false;
	}).detach();
}

void RayTracer::traceChunk(int chunkIndex, const std::vector<Shape*>& worldObjects) {
	// const ThreadChunk& chunk = chunks[chunkIndex];

	for (Shape* object : worldObjects) {
		// Check if Sphere class (I wonder if there's a better way to do this?)
		if (const Sphere* sphere = dynamic_cast<const Sphere*>(object)) {
			intersectSphere(*sphere, chunkIndex);
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

// TODO: Vectorize / use matrix math instead of per ray calculations
void RayTracer::intersectSphere(const Sphere& sphere, int chunkIndex) {
	const ThreadChunk& chunk = chunks[chunkIndex];

	Eigen::Vector3f sphere_center(sphere.position.x, sphere.position.y, sphere.position.z);
	float sphere_radius_sq = sphere.radius * sphere.radius;

	for (int i = chunk.start; i < chunk.end; ++i) {
		// if (ray_steps(0, i) == 0) {
		//  continue;
		// }

		// Fake delay so I can debug
		// std::this_thread::sleep_for(std::chrono::nanoseconds(1));

		Eigen::Vector3f oc = ray_origins.col(i) - sphere_center;
		float a = ray_directions.col(i).squaredNorm();
		float half_b = ray_directions.col(i).dot(oc);
		float c = oc.squaredNorm() - sphere_radius_sq;
		float discriminant = half_b * half_b - a * c;

		if (discriminant > 0) {
			float root = std::sqrt(discriminant);
			float t = (-half_b - root) / a; // Distance from ray origin

			// If hit is in front of camera AND If hit object behind another, we don't care
			if (t > 0.001f && t < t_distance(i)) {
				t_distance(i) = t; // Update closest hit

				Eigen::Vector3f hit_point = ray_origins.col(i) + t * ray_directions.col(i);
				Eigen::Vector3f N = (hit_point - sphere_center).normalized();
				Eigen::Vector3f color = (N + Eigen::Vector3f::Ones()) * 0.5f * 255.0f;
				ray_colors.col(i) << color[0], color[1], color[2];

				ray_steps(0, i) = 0;
			}
		}
	}
}
