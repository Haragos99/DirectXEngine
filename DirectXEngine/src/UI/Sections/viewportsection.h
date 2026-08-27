#pragma once
#include "ipanelsection.h"

// Frame statistics, render mode cycling and background colour.
class ViewportSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Viewport"; }
	void Draw(UIState& state) override;

private:
	bool showDemoWindow = false;
};
