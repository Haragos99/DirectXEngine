#pragma once
#include "ipanelsection.h"
#include "ivoxelsource.h"

// Controls for the marching cubes mesher: pick a volume, how finely to sample
// it and where the surface sits, then extract it into the scene. The section
// hands over an IVoxelSource, so a new volume type only has to be offered here.
class MarchingCubesSection : public IPanelSection
{
public:
	void SetCreateCallback(CreateIsoSurfaceCallback callback) { createIsoSurface = std::move(callback); }

	const char* GetTitle() const override { return "Marching Cubes"; }
	void Draw(UIState& state) override;

private:
	float radius = 1.5f;
	int resolution = 32;
	float isoLevel = 0.0f;
	CreateIsoSurfaceCallback createIsoSurface;
};
