#pragma once
#include "ipanelsection.h"

// Lists the scene objects and owns the selection.
class SceneOutlinerSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Scene objects"; }
	void Draw(UIState& state) override;
};
