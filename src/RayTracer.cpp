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

void RayTracer::initializeRays(Renderer& r) {
	Camera& cam = r.getCamera();
	Transform savedCamTransform = cam.getSavedCamTransform();

	// Offset each ray from top left point on ImagePlane for reference
	const ImagePlane& plane = cam.getImagePlane();
	glm::vec3 quadTopLeft = plane.topLeft();

	// Offset each pixel to ensure ray is centered
	float quadWorldWidth = plane.worldSpaceWidth();
	float pixelWidth = quadWorldWidth / (float)r.getWidth();

	// All rays start at same point initially
	glm::vec3 o = savedCamTransform.position;
	Eigen::Vector3f origin(o.x, o.y, o.z);
	ray_origins = origin.replicate(1, N);

	// Iterate for each pixel in image
	for (int x = 0; x < r.getWidth(); x++) {

		// Pixel offset right
		glm::vec3 offsetRight = plane.transform.right() * (pixelWidth * x);

		for (int y = 0; y < r.getHeight(); y++) {

			// Pixel offset down
			glm::vec3 offsetDown = plane.transform.up() * (pixelWidth * y);
			glm::vec3 posOnImagePlane = quadTopLeft + offsetRight - offsetDown;

			// Convert GLM to Eigen
			Eigen::Vector3f posEigen(posOnImagePlane.x, posOnImagePlane.y, posOnImagePlane.z);

			// Normalize each vector to get a field of unit vectors (each representing cast direction)
			ray_directions.col(x + y * r.getWidth()) = (posEigen - origin).normalized();

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
	ray_steps.setConstant(maxSteps);
	//std::cout << ray_origins << std::endl;
	//std::cout << ray_directions << std::endl;
	exit(0);
}

void RayTracer::traceAll() {
	for (int step = 0; step < maxSteps; ++step) {
		traceStep();
	}
}

void RayTracer::traceStep() {
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
