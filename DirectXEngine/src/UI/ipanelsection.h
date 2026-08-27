#pragma once
#include "uistate.h"

// One self contained block of the control panel. UIPanel owns a list of these
// and draws them in order, so adding a control means adding a section instead
// of growing the panel class.
class IPanelSection
{
public:
	virtual ~IPanelSection() = default;

	// Heading shown above the section.
	virtual const char* GetTitle() const = 0;

	// False hides the section entirely, e.g. splat settings when the selected
	// object has no splats.
	virtual bool IsVisible(const UIState&) const { return true; }

	virtual void Draw(UIState& state) = 0;
};
