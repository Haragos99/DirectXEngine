#pragma once
#include "iscalarfield.h"
#include "progressreporter.h"
#include "voxelgrid.h"

// Turns a continuous scalar field into the discrete grid marching cubes needs.
// Keeping it apart from both means a field never has to know about voxels, and
// a grid filled from a file never has to fake a field.
class ScalarFieldSampler
{
public:
	// `resolution` is the number of samples along the longest axis of the
	// field's bounds; the other axes get cubic cells of the same size.
	static VoxelGrid Sample(const IScalarField& field, int resolution, const ProgressCallback& onProgress = {});
};
