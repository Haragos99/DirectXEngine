#pragma once
#include "ipanelsection.h"

#include <functional>

class JointHandle;
class Skeleton;

// Rig controls: spawns a skeleton, picks the skinning technique and grows the
// rig from the selected joint. The joint tree itself lives in the hierarchy
// section, because a joint is just another scene object.
class SkeletonSection : public IPanelSection
{
public:
	using CreateSkeletonCallback = std::function<void()>;

	void SetCreateCallback(CreateSkeletonCallback callback) { createSkeleton = std::move(callback); }

	const char* GetTitle() const override { return "Skeleton"; }
	void Draw(UIState& state) override;

private:
	// The skeleton the selection belongs to, whether a rig or one of its joints.
	static Skeleton* SelectedSkeleton(const UIState& state);
	static JointHandle* SelectedJoint(const UIState& state);
	void DrawSkinningTechnique(Skeleton& skeleton);

	CreateSkeletonCallback createSkeleton;
};
