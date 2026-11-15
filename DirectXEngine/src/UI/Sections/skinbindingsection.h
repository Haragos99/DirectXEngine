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
	static std::shared_ptr<Skeleton> FindBoundSkeleton(const std::vector<std::shared_ptr<Object3D>>& objects,
	                                                   const Object3D& mesh);

private:
	static void DrawBonePicker(SkinningController& skinning, const Object3D& mesh);
	void DrawSolverParameters(SkinningController& skinning);

	// Index into the collected skeletons, kept across frames.
	int selectedRig = 0;
	std::vector<float> parameterEdits;
};
