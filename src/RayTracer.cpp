#include "RayTracer.h"
#include <chrono>
#include <iostream>
#include <limits>

RayTracer::RayTracer(int numPixels, int maxSteps, int sampleCount)
    : numPixels(numPixels), sampleCount(sampleCount), maxSteps(maxSteps),
      rng(std::random_device{}()), dist(-0.5f, 0.5f) {
	N = numPixels * sampleCount;

	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);
	t_distance.resize(1, N);

	computeChunks();
}

void RayTracer::resize(int newNumPixels) {
	numPixels = newNumPixels;
	N = numPixels * sampleCount;

	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);
	t_distance.resize(1, N);

	computeChunks();
}

void RayTracer::setSampleCount(int samples) {
	sampleCount = samples;
	N = numPixels * sampleCount;

	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);
	t_distance.resize(1, N);

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
	// cam.updateImagePlane((float)r.getWidth(), (float)r.getHeight());

	// Transform savedCamTransform = cam.getSavedCamTransform();
	// glm::vec3 origin = savedCamTransform.position;
	// Eigen::Vector3f cameraOrigin(origin.x, origin.y, origin.z);

	Transform renderCamTransform = cam.getGhostMode() ? cam.getSavedCamTransform() : cam.getCamTransform();
	glm::vec3 origin = renderCamTransform.position;
	Eigen::Vector3f cameraOrigin(origin.x, origin.y, origin.z);

	auto screenWidth = r.getWidth();
	auto screenHeight = r.getHeight();

	// Must have in case user resizes window
	// Also not in glfw resize callback due to performance
	int requiredPixels = screenWidth * screenHeight;
	if (requiredPixels != numPixels) {
		resize(requiredPixels);
	}

	auto plane = cam.getImagePlane();
	auto quadTopLeft = plane.topLeft();
	float quadWorldWidth = plane.worldSpaceWidth();

	// Offset each pixel to ensure ray is centered
	float pixelWidth = quadWorldWidth / (float)screenWidth;

	// Parallelize ray creation
	const unsigned int numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(numThreads);

	// Create chunk to do work on per thread
	int chunk = (screenWidth + numThreads - 1) / numThreads;

	std::cout << "Initializing " << requiredPixels * sampleCount << " rays..." << std::endl;

	for (unsigned int t = 0; t < numThreads; ++t) {
		int startX = t * chunk;
		int endX = std::min(startX + chunk, screenWidth);

		threads.emplace_back([=, this]() {
			std::mt19937 rng_local(std::random_device{}());
			std::uniform_real_distribution<float> dist_local(0.0f, 1.0f);

			for (int x = startX; x < endX; x++) {

				// Pixel offset right
				glm::vec3 offsetRight = plane.transform.right() * (pixelWidth * x);

				for (int y = 0; y < screenHeight; y++) {

					// Pixel offset down
					glm::vec3 offsetDown = plane.transform.up() * (pixelWidth * y);
					glm::vec3 pixelTopLeft = quadTopLeft + offsetRight - offsetDown;

					int pixelIndex = x + y * screenWidth;

					for (int s = 0; s < sampleCount; s++) {
						float randX, randY;

						// If sample count is 1, send through center of pixel
						if (sampleCount == 1) {
							randX = 0.0f;
							randY = 0.0f;
						} else {
							randX = dist_local(rng_local);
							randY = dist_local(rng_local);
						}

						glm::vec3 sampleOffsetRight = plane.transform.right() * (pixelWidth * randX);
						glm::vec3 sampleOffsetDown = plane.transform.up() * (pixelWidth * randY);
						glm::vec3 posOnImagePlane = pixelTopLeft + sampleOffsetRight - sampleOffsetDown;

						Eigen::Vector3f posEigen(posOnImagePlane.x, posOnImagePlane.y, posOnImagePlane.z);

						int rayIndex = pixelIndex * sampleCount + s;
						ray_origins.col(rayIndex) = posEigen;
						ray_directions.col(rayIndex) = (posEigen - cameraOrigin).normalized();
					}
				}
			}
		});
	}

	// Wait for all threads to finish
	for (auto& th : threads)
		th.join();

	std::cout << "Done!" << std::endl;
}

Eigen::Matrix<int, 3, Eigen::Dynamic> RayTracer::getAveragedColors() const {
	Eigen::Matrix<int, 3, Eigen::Dynamic> averaged_colors(3, numPixels);
	averaged_colors.setZero();

	// TODO: This takes a while if scaled up to around 2440x1440 (~55ms)
	// std::cout << "Start" << std::endl;
	// auto start = std::chrono::high_resolution_clock::now();

	for (int pixelIdx = 0; pixelIdx < numPixels; pixelIdx++) {
		Eigen::Vector3f colorSum(0.0f, 0.0f, 0.0f);

		for (int s = 0; s < sampleCount; s++) {
			int rayIndex = pixelIdx * sampleCount + s;
			colorSum += ray_colors.col(rayIndex).cast<float>();
		}

		Eigen::Vector3f avgColor = colorSum / (float)sampleCount;
		averaged_colors.col(pixelIdx) = avgColor.cast<int>();
	}

	// auto end = std::chrono::high_resolution_clock::now();
	// auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << duration.count() << std::endl;
	// std::cout << "Done" << std::endl;

	return averaged_colors;
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
		// if (const Square* square = dynamic_cast<const Square*>(object)) {
		//	intersectSquare(*square, chunkIndex);
		// }
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
		//	continue;
		// }

		// Fake delay so I can debug
		std::this_thread::sleep_for(std::chrono::microseconds(1));

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
				                   //
				// TODO: Move this outside of specific shape method
				switch (sphere.getMaterial()) {
				case Material::DIFFUSE:
					ray_colors.col(i) << 0.0f, 255.0f, 255.0f;
					break;
				default: // Normal
					Eigen::Vector3f hit_point = ray_origins.col(i) + t * ray_directions.col(i);
					Eigen::Vector3f N = (hit_point - sphere_center).normalized();
					Eigen::Vector3f color = (N + Eigen::Vector3f::Ones()) * 0.5f * 255.0f;
					ray_colors.col(i) << color[0], color[1], color[2];
				};

				ray_steps(0, i) = 0;
			}
		}
	}
}
