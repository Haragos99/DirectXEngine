#pragma once
#include "progressreporter.h"
#include "transform.h"
#include "voxelgrid.h"

#include <functional>
#include <memory>

// Anything that can be turned into a voxel volume for the mesher. It is the
// only thing the meshing entry point has to know about, so an implicit field, a
// Gaussian splat capture or a scan all reach marching cubes the same way.
class IVoxelSource
{
public:
	virtual ~IVoxelSource() = default;

	// Name used for the generated scene object.
	virtual const char* GetName() const = 0;

	// Voxelises the volume. `resolution` is the sample count along its longest
	// axis; the shorter axes get cells of the same size.
	virtual VoxelGrid Build(int resolution, const ProgressCallback& onProgress) const = 0;

	// Where the generated mesh belongs, because a volume is defined in the space
	// of whatever produced it rather than in world space.
	virtual Transform GetPlacement() const { return Transform(); }
};

// Handed to the panel sections so they can ask for a volume to be meshed.
using CreateIsoSurfaceCallback =
	std::function<void(std::shared_ptr<IVoxelSource>, int resolution, float isoLevel)>;
