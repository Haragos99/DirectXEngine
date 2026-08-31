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
	GizmoMode gizmoMode = GizmoMode::Move;
	float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
	bool renderModeChangeRequested = false;

	size_t SceneObjectCount() const;
	bool HasSelection() const;
	std::shared_ptr<Object3D> SelectedObject() const { return selection.lock(); }
	bool IsSelected(const std::shared_ptr<Object3D>& object) const;
	// Selecting anything tells the old and the new object about it, which is how
	// a joint handle highlights its joint.
	void Select(const std::shared_ptr<Object3D>& object);

private:
	// Weak, so an object removed from the scene drops out of the selection.
	std::weak_ptr<Object3D> selection;
};
