#include "skeletonsection.h"
#include "jointhandle.h"
#include "skeleton.h"

#include "imgui.h"

Skeleton* SkeletonSection::SelectedSkeleton(const UIState& state)
{
	Object3D* selected = state.SelectedObject().get();
	if (auto* skeleton = dynamic_cast<Skeleton*>(selected))
		return skeleton;
	if (auto* handle = dynamic_cast<JointHandle*>(selected))
		return &handle->GetSkeleton();
	return nullptr;
}

JointHandle* SkeletonSection::SelectedJoint(const UIState& state)
{
	return dynamic_cast<JointHandle*>(state.SelectedObject().get());
}

void SkeletonSection::Draw(UIState& state)
{
	if (ImGui::Button("Create skeleton") && createSkeleton)
		createSkeleton();

	Skeleton* skeleton = SelectedSkeleton(state);
	if (skeleton == nullptr)
	{
		ImGui::TextDisabled("Select a skeleton or one of its joints in the hierarchy.");
		return;
	}

	// Shown here too because the skin binding panel is hidden while a joint is
	// selected, which is exactly when the rig is being posed.
	const SkinningController& skinning = skeleton->GetSkinning();
	const size_t boundMeshes = skinning.GetBoundMeshCount();
	if (boundMeshes == 0)
		ImGui::TextDisabled("Skinning: %s, no mesh bound", skinning.GetMethodName());
	else
		ImGui::TextColored(ImVec4(0.45f, 0.90f, 0.45f, 1.0f), "Skinning: %s, %zu mesh%s bound",
		                   skinning.GetMethodName(), boundMeshes, boundMeshes == 1 ? "" : "es");

	// With the rig itself selected, the new joint grows from the root.
	JointHandle* joint = SelectedJoint(state);
	const int parentJoint = joint ? joint->GetJointId() : 0;

	if (ImGui::Button("Add child joint"))
		state.Select(skeleton->AddJoint(parentJoint));
	ImGui::SameLine();

	if (ImGui::Button("Reset pose"))
		skeleton->ResetPose();
}
