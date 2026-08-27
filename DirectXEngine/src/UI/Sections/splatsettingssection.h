#pragma once
#include "ipanelsection.h"

class SplatCloudObject;

// Size control for the .ply clouds. Only visible while a Gaussian splat capture
// or a point cloud is selected, because nothing else has a splat radius to scale.
class SplatSettingsSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Splat size"; }
	bool IsVisible(const UIState& state) const override;
	void Draw(UIState& state) override;

private:
	// Selected object as a splat cloud, or nullptr when it is anything else.
	static SplatCloudObject* AsSplatCloud(const UIState& state);
};
