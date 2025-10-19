#include "RayTracer.h"
#include <chrono>
#include <iostream>
#include <limits>

RayTracer::RayTracer(int numPixels, int maxBounces, int sampleCount)
    : numPixels(numPixels), targetSampleCount(sampleCount), maxBounces(maxBounces),
      rng(std::random_device{}()), dist(-0.5f, 0.5f) {
	N = numPixels;

	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);
	t_distance.resize(1, N);

	accumulated_buffer_a.resize(3, numPixels);
	accumulated_buffer_b.resize(3, numPixels);
	accumulated_buffer_a.setZero();
	accumulated_buffer_b.setZero();

	display_buffer = &accumulated_buffer_b;

	computeChunks();
}

void RayTracer::resize(int newNumPixels) {
	numPixels = newNumPixels;
	N = numPixels;

	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);
	t_distance.resize(1, N);

	accumulated_buffer_a.resize(3, numPixels);
	accumulated_buffer_b.resize(3, numPixels);
	accumulated_buffer_a.setZero();
	accumulated_buffer_b.setZero();

	display_buffer.store(&accumulated_buffer_b);
	display_sample_count.store(0);
	currentSampleCount = 0;

	computeChunks();
}

void RayTracer::setSampleCount(int samples) {
	targetSampleCount = samples;
}

void RayTracer::computeChunks() {
	chunks.clear();
	int raysPerThread = N / NUM_THREADS;
	int remainder = N % NUM_THREADS;

	int start = 0;
	for (int i = 0; i < NUM_THREADS; i++) {
		int chunkSize = raysPerThread + (i < remainder ? 1 : 0);
		chunks.push_back({start, start + chunkSize});
		start += chunkSize;
	}
}

void RayTracer::initializeRays(Renderer& r, int sampleIndex) {
	ray_steps.setConstant(maxBounces);
	ray_colors.setConstant(255);
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

	std::cout << "Initializing rays for sample " << sampleIndex << std::endl;

	for (unsigned int t = 0; t < numThreads; t++) {
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

					float randX, randY;

					// If sample count is 1, send through center of pixel
					if (targetSampleCount == 1) {
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

					ray_origins.col(pixelIndex) = posEigen;
					ray_directions.col(pixelIndex) = (posEigen - cameraOrigin).normalized();
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

	int samples = display_sample_count.load();
	const Eigen::Matrix<float, 3, Eigen::Dynamic>* buffer_to_read = display_buffer.load();

	if (samples == 0 || buffer_to_read == nullptr) {
		averaged_colors.setZero();
		return averaged_colors;
	}

	for (int pixelIdx = 0; pixelIdx < numPixels; pixelIdx++) {
		Eigen::Vector3f avgColor = (*buffer_to_read).col(pixelIdx) / (float)samples;
		averaged_colors.col(pixelIdx) = avgColor.cast<int>();
	}

	return averaged_colors;
}

void RayTracer::traceAllAsync(const std::vector<Shape*>& worldObjects, Renderer& renderer) {
	// We don't want trace if it's already tracing
	if (isTracing())
		return;

	tracing = true;

	// Start in own separate thread so we can see it real-time
	std::thread([this, &worldObjects, &renderer]() {
		// Reset buffer states
		accumulated_buffer_a.setZero();
		accumulated_buffer_b.setZero();
		currentSampleCount = 0;

		Eigen::Matrix<float, 3, Eigen::Dynamic>* current_write_ptr = &accumulated_buffer_a;
		const Eigen::Matrix<float, 3, Eigen::Dynamic>* current_read_ptr = &accumulated_buffer_b;

		// Init
		display_buffer.store(current_read_ptr);
		display_sample_count.store(0);

		// Sequential anti aliasing
		for (int sample = 0; sample < targetSampleCount; sample++) {
			initializeRays(renderer, sample);

			// Bounces
			for (int bounce = 0; bounce < maxBounces; bounce++) {
				std::cout << "Bounce: " << bounce << std::endl;
				for (int i = 0; i < N; i++) {
					if (ray_steps(0, i) > 0) {
						t_distance(i) = std::numeric_limits<float>::infinity();
					}
				}

				std::vector<std::thread> threads;
				for (int i = 0; i < NUM_THREADS; i++) {
					threads.emplace_back(&RayTracer::traceChunk, this, i, worldObjects);
				}
				for (auto& t : threads) {
					t.join();
				}

				for (int i = 0; i < N; i++) {
					if (ray_steps(0, i) > 0 && t_distance(i) == std::numeric_limits<float>::infinity()) {
						Eigen::Vector3f dir = ray_directions.col(i).normalized();
						float t = 0.5f * (dir.y() + 1.0f);

						// Simple sky gradient
						Eigen::Vector3f sky_color = (1.0f - t) * Eigen::Vector3f(1.0f, 1.0f, 1.0f) + t * Eigen::Vector3f(0.5f, 0.7f, 1.0f);
						Eigen::Vector3f final_color = (ray_colors.col(i).cast<float>().array() / 255.0f * sky_color.array() * 255.0f).matrix();

						ray_colors.col(i) = final_color.cast<int>();
						ray_steps(0, i) = 0;
					}
				}

				// Check if all rays are done
				if (ray_steps.isZero())
					break;
			}

			// Accumulate sample
			for (int i = 0; i < numPixels; ++i) {
				(*current_write_ptr).col(i) = (*current_read_ptr).col(i) + ray_colors.col(i).cast<float>();
			}
			currentSampleCount++;

			display_buffer.store(current_write_ptr);
			display_sample_count.store(currentSampleCount);

			// Swap pointers for the next pass
			std::swap(current_write_ptr, *const_cast<Eigen::Matrix<float, 3, Eigen::Dynamic>**>(&current_read_ptr));

			std::cout << "Completed sample " << currentSampleCount << std::endl;
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
	std::mt19937 rng_local(std::random_device{}());
	std::uniform_real_distribution<float> dist_local(-1.0f, 1.0f);

	const ThreadChunk& chunk = chunks[chunkIndex];

	Eigen::Vector3f sphere_center(sphere.position.x, sphere.position.y, sphere.position.z);
	float sphere_radius_sq = sphere.radius * sphere.radius;

	for (int i = chunk.start; i < chunk.end; ++i) {
		if (ray_steps(0, i) == 0) {
			continue;
		}

		// Fake delay so I can debug
		// std::this_thread::sleep_for(std::chrono::microseconds(1));

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

				// TODO: Move this outside of specific shape method
				switch (sphere.getMaterial()) {
				case Material::DIFFUSE: {
					Eigen::Vector3f random_vec;
					float lensq;
					do {
						random_vec = Eigen::Vector3f(dist_local(rng_local), dist_local(rng_local), dist_local(rng_local));
						lensq = random_vec.squaredNorm();
					} while (lensq > 1.0f || lensq < 1e-40f);

					Eigen::Vector3f unit_vec = random_vec / std::sqrt(lensq);

					if (unit_vec.dot(N) < 0.0f) {
						unit_vec = -unit_vec;
					}

					ray_origins.col(i) = hit_point;
					ray_directions.col(i) = unit_vec;

					ray_colors.col(i) = (ray_colors.col(i).cast<float>() * 0.5f).cast<int>();

					ray_steps(0, i) = ray_steps(0, i) - 1;
					break;
				}
				default: { // Normal
					Eigen::Vector3f color = (N + Eigen::Vector3f::Ones()) * 0.5f * 255.0f;
					ray_colors.col(i) = color.cast<int>();
					ray_steps(0, i) = 0;
					break;
				}
				};
			}
		}
	}
}
