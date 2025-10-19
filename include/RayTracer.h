#pragma once

#include "Material.h"
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
	int N;         // Num of rays (diff bc numPixels*sampleCount)
	int numPixels; // Actual number of pixels
	int targetSampleCount;
	int currentSampleCount;
	int maxBounces;
	int NUM_THREADS{64};
	std::atomic<bool> tracing{false}; // We want this to be atomic since it's being assigned within multiple threads

	Eigen::Array<int, 1, Eigen::Dynamic> ray_steps;               // Lifecycle of each ray
	Eigen::Matrix<int, 3, Eigen::Dynamic> ray_colors;             // Final color to be rendered on ImagePlane texture
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_origins;          // Position of each ray
	Eigen::Matrix<float, 3, Eigen::Dynamic> ray_directions;       // Direction of each ray (normalized)
	Eigen::Matrix<float, 1, Eigen::Dynamic> t_distance;           // Tracks closest hit (prevents rendering mistakes due to execution order)
	Eigen::Matrix<float, 3, Eigen::Dynamic> accumulated_buffer_a; // Double buffered
	Eigen::Matrix<float, 3, Eigen::Dynamic> accumulated_buffer_b;

	// We need this since we are dealing with multiple threads + rendering
	std::atomic<const Eigen::Matrix<float, 3, Eigen::Dynamic>*> display_buffer;
	std::atomic<int> display_sample_count{0};

	// For random sampling
	std::mt19937 rng;
	std::uniform_real_distribution<float> dist;

  public:
	// Init
	RayTracer(int numPixels, int maxBounces, int sampleCount = 1);
	void initializeRays(Renderer&, int sampleIndex);
	void resize(int numPixels);
	void setSampleCount(int samples);

	// For multithreading
	std::vector<ThreadChunk> chunks;
	void computeChunks();
	void traceChunk(int chunkIndex, const std::vector<Shape*>& worldObjects);

	// Trace
	void traceAllAsync(const std::vector<Shape*>& worldObjects, Renderer& renderer);
	void traceStep();

	// Color averaging
	Eigen::Matrix<int, 3, Eigen::Dynamic> getAveragedColors() const;

	// Getters
	const Eigen::Array<int, 1, Eigen::Dynamic>& getRaySteps() const { return ray_steps; }
	const Eigen::Matrix<float, 3, Eigen::Dynamic>& getRayOrigins() const { return ray_origins; }
	const Eigen::Matrix<float, 3, Eigen::Dynamic>& getRayDirections() const { return ray_directions; }
	int getCurrentSampleCount() const { return currentSampleCount; }

	int getNumRays() const { return N; }
	int getNumPixels() const { return numPixels; }
	int getMaxSteps() const { return maxBounces; }
	int getNumThreads() const { return NUM_THREADS; }
	int isTracing() const { return tracing; }

	// Intersections
	void intersectSphere(const Sphere&, int chunkIndex);
	void intersectSquare(const Square&, int chunkIndex);
};
