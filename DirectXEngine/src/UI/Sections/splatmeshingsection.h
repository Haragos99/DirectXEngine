#pragma once
#include "ipanelsection.h"
#include "ivoxelsource.h"

class SplatCloudObject;

// Turns the selected Gaussian splat capture into a polygonal mesh. Only visible
// while a splat cloud is selected, because nothing else carries a density to
// voxelise.
class SplatMeshingSection : public IPanelSection
{
public:
	void SetCreateCallback(CreateIsoSurfaceCallback callback) { createIsoSurface = std::move(callback); }

	const char* GetTitle() const override { return "Splat meshing"; }
	bool IsVisible(const UIState& state) const override;
	void Draw(UIState& state) override;

private:
	// Selected object as a splat cloud, or nullptr when it is anything else.
	static SplatCloudObject* AsSplatCloud(const UIState& state);

	int resolution = 96;
	float isoLevel = 0.15f;
	float reach = 2.0f;
	CreateIsoSurfaceCallback createIsoSurface;
};
