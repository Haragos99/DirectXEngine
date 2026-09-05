#pragma once
#include "imodelloader.h"
#include "ivoxelsource.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

// Runs the voxelise + marching cubes pass on a worker thread.
//
// Both steps are pure CPU work on data the renderer never touches, so they can
// leave the frame alone; only the finished MeshData has to come back to the
// main thread, which is where the GPU buffers get created.
class IsoSurfaceJob
{
public:
	IsoSurfaceJob(std::shared_ptr<IVoxelSource> volume, int resolution, float isoLevel);
	// Joins the worker, so destroying a job never outruns it.
	~IsoSurfaceJob();

	IsoSurfaceJob(const IsoSurfaceJob&) = delete;
	IsoSurfaceJob& operator=(const IsoSurfaceJob&) = delete;

	bool IsFinished() const { return finished.load(std::memory_order_acquire); }
	// 0..1 across both steps.
	float GetProgress() const { return progress.load(std::memory_order_relaxed); }
	const std::string& GetLabel() const { return label; }

	// Only valid once IsFinished() is true.
	MeshData TakeMesh() { return std::move(mesh); }
	std::string GetMeshName() const { return volume->GetName() + std::string(" Isosurface"); }
	Transform GetPlacement() const { return volume->GetPlacement(); }

private:
	void Run(int resolution, float isoLevel);

	std::shared_ptr<IVoxelSource> volume;
	std::string label;
	MeshData mesh;
	std::atomic<float> progress{ 0.0f };
	std::atomic<bool> finished{ false };
	std::thread worker;
};
