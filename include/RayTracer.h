#pragma once

#include "Renderer.h"
#include "Sphere.h"
#include <Eigen/Core>
#include <cmath>
#include <limits>
#include <vector>

class RayTracer {
  private:
	int N; // Num of rays
	int maxSteps;

	Eigen::Array<int, 1, Eigen::Dynamic> ray_steps;
	Eigen::Matrix<int, 3, Eigen::Dynamic> ray_colors;
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_origins;
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_directions;

  public:
	// Init
	RayTracer(int numRays, int maxSteps);
	void initializeRays(Renderer&);

	// Trace
	void traceAll();
	void traceStep();

	// Shapes
	void intersectSphere(const Sphere&);
	// void intersectTriangle();
};
