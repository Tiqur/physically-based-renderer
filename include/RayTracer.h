#pragma once

#include "Renderer.h"
#include "Sphere.h"
#include "Square.h"
#include <Eigen/Core>
#include <cmath>
#include <limits>
#include <random>
#include <thread>
#include <vector>

class Renderer;

struct ThreadChunk {
	int start;
	int end;
};

class RayTracer {
  private:
	int N;           // Num of rays (diff bc numPixels*sampleCount)
	int numPixels;   // Actual number of pixels
	int sampleCount; // Number of samples/rays per pixel
	int maxSteps;
	int NUM_THREADS = 64;
	std::atomic<bool> tracing{false}; // We want this to be atomic since it's being assigned within multiple threads

	Eigen::Array<int, 1, Eigen::Dynamic> ray_steps;         // Lifecycle of each ray
	Eigen::Matrix<int, 3, Eigen::Dynamic> ray_colors;       // Final color to be rendered on ImagePlane texture
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_origins;    // Position of each ray
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_directions; // Direction of each ray (normalized)
	Eigen::Matrix<float, 1, Eigen::Dynamic> t_distance;     // Tracks closest hit (prevents rendering mistakes due to execution order)

	// For random sampling
	std::mt19937 rng;
	std::uniform_real_distribution<float> dist;

  public:
	// Init
	RayTracer(int numPixels, int maxSteps, int sampleCount = 1);
	void initializeRays(Renderer&);
	void resize(int numPixels);
	void setSampleCount(int samples);

	// For multithreading
	std::vector<ThreadChunk> chunks;
	void computeChunks();
	void traceChunk(int chunkIndex, const std::vector<Shape*>& worldObjects);

	// Trace
	void traceAllAsync(const std::vector<Shape*>& worldObjects);
	void traceStep();

	// Color averaging
	Eigen::Matrix<int, 3, Eigen::Dynamic> getAveragedColors() const;

	// Getters
	const Eigen::Array<int, 1, Eigen::Dynamic>& getRaySteps() const { return ray_steps; }
	const Eigen::Matrix<int, 3, Eigen::Dynamic>& getRayColors() const { return ray_colors; }
	const Eigen::Matrix<float, 3, Eigen::Dynamic>& getRayOrigins() const { return ray_origins; }
	const Eigen::Matrix<float, 3, Eigen::Dynamic>& getRayDirections() const { return ray_directions; }

	int getNumRays() const { return N; }
	int getNumPixels() const { return numPixels; }
	int getSampleCount() const { return sampleCount; }
	int getMaxSteps() const { return maxSteps; }
	int getNumThreads() const { return NUM_THREADS; }
	int isTracing() const { return tracing; }

	// Intersections
	void intersectSphere(const Sphere&, int chunkIndex);
	void intersectSquare(const Square&, int chunkIndex);
};
