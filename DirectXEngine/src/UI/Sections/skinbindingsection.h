#pragma once
#include "ipanelsection.h"

#include <memory>
#include <vector>

class Object3D;
class Skeleton;
class SkinningController;

// Binds the selected mesh to a rig: pick a skeleton, press Bind, and the rig
// deforms the mesh from then on. Only shown while a skinnable mesh is selected.
class SkinBindingSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Skin Binding"; }
	bool IsVisible(const UIState& state) const override;
	void Draw(UIState& state) override;

	// Every skeleton in the scene, however deeply it is parented.
	static void CollectSkeletons(const std::vector<std::shared_ptr<Object3D>>& objects,
	                             std::vector<std::shared_ptr<Skeleton>>& found);
	// The skeleton `mesh` is bound to, or null.
	static std::shared_ptr<Skeleton> FindBoundSkeleton(const std::vector<std::shared_ptr<Object3D>>& objects,
	                                                   const Object3D& mesh);

private:
	// Which bone the single-bone weight view shows, each with its bone colour.
	static void DrawBonePicker(SkinningController& skinning, const Object3D& mesh);
	// One slider per parameter the selected weight solver declares.
	void DrawSolverParameters(SkinningController& skinning);

	// Index into the collected skeletons, kept across frames.
	int selectedRig = 0;
	// Edited copies of the solver's parameters: solving is too costly to redo on
	// every frame of a drag, so the controller only hears about them once the
	// drag ends.
	std::vector<float> parameterEdits;
};
