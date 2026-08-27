#pragma once
#include "object3d.h"
#include "gizmomode.h"

#include <memory>
#include <vector>

// Everything the control panel sections read and write. UIPanel owns it and
// re-exposes the parts the engine needs, so sections never talk to each other.
struct UIState
{
	// Borrowed for the duration of one frame; never owned by the UI.
	const std::vector<std::shared_ptr<Object3D>>* sceneObjects = nullptr;
	int selectedIndex = -1;
	GizmoMode gizmoMode = GizmoMode::Move;
	float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
	bool renderModeChangeRequested = false;

	size_t SceneObjectCount() const;
	bool HasSelection() const;
	std::shared_ptr<Object3D> SelectedObject() const;
	// Forgets a selection that no longer exists, e.g. after the scene shrank.
	void ValidateSelection();
};
