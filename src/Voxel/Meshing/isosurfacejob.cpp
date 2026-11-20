#include "isosurfacejob.h"
#include "marchingcubes.h"

#include <utility>

namespace
{
	// Voxelising dominates the wall clock, so it owns most of the bar.
	constexpr float kVoxeliseShare = 0.7f;
}

IsoSurfaceJob::IsoSurfaceJob(std::shared_ptr<IVoxelSource> _volume, int resolution, float isoLevel)
	: volume(std::move(_volume))
	, label(std::string("Meshing ") + volume->GetName())
{
	worker = std::thread(&IsoSurfaceJob::Run, this, resolution, isoLevel);
}

IsoSurfaceJob::~IsoSurfaceJob()
{
	if (worker.joinable())
		worker.join();
}

void IsoSurfaceJob::Run(int resolution, float isoLevel)
{
	const VoxelGrid grid = volume->Build(resolution, [this](float fraction) {
		progress.store(fraction * kVoxeliseShare, std::memory_order_relaxed);
	});

	mesh = MarchingCubes().Generate(grid, isoLevel, [this](float fraction) {
		progress.store(kVoxeliseShare + fraction * (1.0f - kVoxeliseShare), std::memory_order_relaxed);
	});

	progress.store(1.0f, std::memory_order_relaxed);
	// Release pairs with the acquire in IsFinished, publishing `mesh`.
	finished.store(true, std::memory_order_release);
}
