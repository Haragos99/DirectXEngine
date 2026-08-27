#pragma once
#include "ipanelsection.h"

// Chooses which gizmo the viewport manipulates with. It stays available whether
// or not something is selected, because it decides what a future selection does.
class TransformModeSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Transform mode"; }
	void Draw(UIState& state) override;
};
