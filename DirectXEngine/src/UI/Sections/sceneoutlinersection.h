#pragma once
#include "ipanelsection.h"

#include <functional>
#include <memory>

// The one hierarchy view of the editor: every scene object, with its children
// nested underneath. Owns the selection and lets rows be dragged onto each
// other to reparent them, so joints and meshes are handled the same way.
class SceneOutlinerSection : public IPanelSection
{
public:
	// child, new parent. A null parent moves the object back to the scene root.
	using ReparentCallback = std::function<void(std::shared_ptr<Object3D>, std::shared_ptr<Object3D>)>;

	void SetReparentCallback(ReparentCallback callback) { reparent = std::move(callback); }

	const char* GetTitle() const override { return "Hierarchy"; }
	void Draw(UIState& state) override;

private:
	void DrawNode(UIState& state, const std::shared_ptr<Object3D>& object);
	// Makes a row a drag source and a drop target for reparenting.
	void DrawDragAndDrop(const std::shared_ptr<Object3D>& object);

	ReparentCallback reparent;
	// Kept alive between the drag start and the drop.
	std::weak_ptr<Object3D> dragged;
};
