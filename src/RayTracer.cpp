#include "RayTracer.h"
#include <iostream>

RayTracer::RayTracer(int numRays, int maxSteps) : N(numRays), maxSteps(maxSteps) {
	ray_origins.resize(3, N);
	ray_directions.resize(3, N);
	ray_colors.resize(3, N);
	ray_steps.resize(1, N);

	ray_colors.setZero();
	ray_steps.setConstant(maxSteps);
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

void RayTracer::traceAll() {
	for (int step = 0; step < maxSteps; ++step) {
		traceStep();
	}
}

void RayTracer::traceStep() {

	// TODO: Parallelize across N threads
	for (int i = 0; i < N; ++i) {
		if (ray_steps(0, i) < 1)
			continue; // inactive ray
		ray_steps(0, i)--;
		std::cout << "Tracing ray " << i << " step " << ray_steps(0, i) << "\n";

		// TODO: Add intersections here
	}
}

void RayTracer::intersectSphere(const Sphere& sphere) {
	(void)sphere;
	return;
}
