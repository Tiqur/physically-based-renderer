#pragma once

#include "Renderer.h"
#include "Sphere.h"
#include <Eigen/Core>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

class Renderer;

struct ThreadChunk {
	int start;
	int end;
};

class RayTracer {
  private:
	int N; // Num of rays
	int maxSteps;
	int NUM_THREADS = 16;
	std::atomic<bool> tracing{false}; // We want this to be atomic since it's being assigned within multiple threads

	Eigen::Array<int, 1, Eigen::Dynamic> ray_steps;
	Eigen::Matrix<int, 3, Eigen::Dynamic> ray_colors;
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_origins;
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_directions;

  public:
	// Init
	RayTracer(int numRays, int maxSteps);
	void initializeRays(Renderer&);
	void resize(int numRays);

	// For multithreading
	std::vector<ThreadChunk> chunks;
	void computeChunks();
	void traceChunk(int chunkIndex);

	// Trace
	void traceAllAsync();
	void traceStep();

	// Shapes
	void intersectSphere(const Sphere&);
	// void intersectTriangle();

	// Getters
	const Eigen::Array<int, 1, Eigen::Dynamic>& getRaySteps() const { return ray_steps; }
	const Eigen::Matrix<int, 3, Eigen::Dynamic>& getRayColors() const { return ray_colors; }
	const Eigen::Matrix<float, 3, Eigen::Dynamic>& getRayOrigins() const { return ray_origins; }
	const Eigen::Matrix<float, 3, Eigen::Dynamic>& getRayDirections() const { return ray_directions; }

	int getNumRays() const { return N; }
	int getMaxSteps() const { return maxSteps; }
	int getNumThreads() const { return NUM_THREADS; }
	int isTracing() const { return tracing; }
};
