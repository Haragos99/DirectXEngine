#pragma once
#include "imodelloader.h"
#include "progressreporter.h"
#include "voxelgrid.h"

// Marching cubes surface extraction: walks every cell of a voxel grid, looks up
// which of its twelve edges the iso surface crosses and emits the triangles for
// that configuration.
//
// It depends on VoxelGrid alone, so it meshes any volume - the sphere demo now,
// a Gaussian splat density later - without a change.
class MarchingCubes
{
public:
	// Triangle mesh of the surface where the grid equals `isoLevel`. The result
	// is empty when the surface does not cross the volume.
	MeshData Generate(const VoxelGrid& grid, float isoLevel, const ProgressCallback& onProgress = {}) const;

private:
	// Index of the shared vertex on `edge` of cell (x, y, z), creating and
	// interpolating it the first time a cell asks for it. `cache` holds only the
	// two grid slices the current cell slice can reach.
	static UINT EdgeVertex(const VoxelGrid& grid, float isoLevel,
	                       int x, int y, int z, int edge,
	                       std::vector<int>& cache, MeshData& out);
};
