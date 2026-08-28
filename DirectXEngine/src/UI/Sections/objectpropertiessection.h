#pragma once
#include "ipanelsection.h"

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
