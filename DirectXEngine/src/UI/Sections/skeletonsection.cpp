#include "skeletonsection.h"
#include "dualquaternionskinning.h"
#include "jointhandle.h"
#include "linearblendskinning.h"
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

	DrawSkinningTechnique(*skeleton);

	if (JointHandle* joint = SelectedJoint(state))
	{
		if (ImGui::Button("Add child joint"))
			state.Select(skeleton->AddJoint(joint->GetJointId()));
		ImGui::SameLine();
	}
	else
	{
		ImGui::TextDisabled("Pick a joint to grow the rig or to drag it with the gizmo.");
	}

	if (ImGui::Button("Reset pose"))
		skeleton->ResetPose();
}

void SkeletonSection::DrawSkinningTechnique(Skeleton& skeleton)
{
	if (!ImGui::BeginCombo("Skinning", skeleton.GetSkinningTechnique()->GetName()))
		return;

	if (ImGui::Selectable("Linear Blend (LBS)"))
		skeleton.SetSkinningTechnique(std::make_shared<LinearBlendSkinning>());
	if (ImGui::Selectable("Dual Quaternion (DQS)"))
		skeleton.SetSkinningTechnique(std::make_shared<DualQuaternionSkinning>());

	ImGui::EndCombo();
}
