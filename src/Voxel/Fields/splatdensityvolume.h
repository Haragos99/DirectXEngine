#pragma once
#include "ivoxelsource.h"
#include "vertex.h"

#include <string>
#include <vector>

// Voxelises a Gaussian splat capture into a density volume, which is what lets
// marching cubes turn a cloud into a surface.
//
// Each splat is scattered into the cells it reaches rather than each cell
// asking which splats cover it: with a million anisotropic Gaussians the second
// direction would be hopeless. Cells keep the strongest contribution, so the
// result is the union of the ellipsoids and the iso level reads as an opacity.
class SplatDensityVolume : public IVoxelSource
{
public:
	SplatDensityVolume(std::vector<SplatInstance> splats,
	                   const DirectX::XMFLOAT3& boundsMin,
	                   const DirectX::XMFLOAT3& boundsMax,
	                   float reachInDeviations,
	                   const Transform& placement,
	                   std::string sourceName);

	const char* GetName() const override { return name.c_str(); }
	VoxelGrid Build(int resolution, const ProgressCallback& onProgress) const override;
	Transform GetPlacement() const override { return placement; }

private:
	void Scatter(const SplatInstance& splat, VoxelGrid& grid) const;

	std::vector<SplatInstance> splats;
	DirectX::XMFLOAT3 boundsMin;
	DirectX::XMFLOAT3 boundsMax;
	// How far out, in standard deviations, a splat still contributes.
	float reachInDeviations;
	Transform placement;
	std::string name;
};
