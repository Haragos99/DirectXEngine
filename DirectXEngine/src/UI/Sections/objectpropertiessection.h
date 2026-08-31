#pragma once
#include "ipanelsection.h"

// Inspector for the selected object. Shows its absolute translation, rotation
// and scale, and lets them be typed or dragged in like in a 3D editor. Hidden
// while nothing is selected, because there is nothing to inspect then.
class ObjectPropertiesSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Object Properties"; }
	bool IsVisible(const UIState& state) const override;
	void Draw(UIState& state) override;

private:
	// Locks the three scale axes together, like the chain toggle of a 3D editor.
	bool uniformScale = false;
};
