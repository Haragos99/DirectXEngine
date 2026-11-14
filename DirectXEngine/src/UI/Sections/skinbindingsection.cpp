#include "skinbindingsection.h"

#include "skeleton.h"
#include "uistate.h"

#include "imgui.h"

namespace
{
	const ImVec4 kActiveColor(0.45f, 0.90f, 0.45f, 1.0f);
	const ImVec4 kPendingColor(0.90f, 0.78f, 0.35f, 1.0f);

	constexpr SkinningController::Method kMethods[] = {
		SkinningController::Method::LinearBlend,
		SkinningController::Method::DualQuaternion,
	};

	constexpr SkinningController::WeightMode kWeightModes[] = {
		SkinningController::WeightMode::Default,
		SkinningController::WeightMode::BoneHeat,
	};

	constexpr float kMinAlpha = 0.0001f;
	constexpr float kMaxAlpha = 10.0f;
}

bool SkinBindingSection::IsVisible(const UIState& state) const
{
	const std::shared_ptr<Object3D> selected = state.SelectedObject();
	return selected && selected->GetSkinVertices() != nullptr;
}

void SkinBindingSection::CollectSkeletons(const std::vector<std::shared_ptr<Object3D>>& objects,
                                          std::vector<std::shared_ptr<Skeleton>>& found)
{
	for (const std::shared_ptr<Object3D>& object : objects)
	{
		if (auto rig = std::dynamic_pointer_cast<Skeleton>(object))
			found.push_back(rig);

		CollectSkeletons(object->GetChildren(), found);
	}
}

void SkinBindingSection::Draw(UIState& state)
{
	const std::shared_ptr<Object3D> mesh = state.SelectedObject();
	if (!mesh || state.sceneObjects == nullptr)
		return;

	std::vector<std::shared_ptr<Skeleton>> rigs;
	CollectSkeletons(*state.sceneObjects, rigs);

	if (rigs.empty())
	{
		ImGui::TextDisabled("Create a skeleton first.");
		return;
	}

	ImGui::Text("Mesh: %s", mesh->GetName().c_str());

	if (selectedRig >= static_cast<int>(rigs.size()))
		selectedRig = 0;

	// A mesh belongs to at most one rig, so the bound one is what we act on.
	Skeleton* boundTo = nullptr;
	for (const std::shared_ptr<Skeleton>& rig : rigs)
	{
		if (rig->GetSkinning().IsBound(*mesh))
			boundTo = rig.get();
	}

	// Binding fixes the rig; until then it is whichever one is being picked.
	Skeleton& target = boundTo ? *boundTo : *rigs[selectedRig];

	if (boundTo != nullptr)
	{
		ImGui::Text("Skeleton: %s", boundTo->GetName().c_str());
	}
	else if (ImGui::BeginCombo("Skeleton", rigs[selectedRig]->GetName().c_str()))
	{
		for (int i = 0; i < static_cast<int>(rigs.size()); ++i)
		{
			if (ImGui::Selectable(rigs[i]->GetName().c_str(), i == selectedRig))
				selectedRig = i;
		}
		ImGui::EndCombo();
	}

	SkinningController& skinning = target.GetSkinning();

	// Offered before binding too, so the very first deform already uses the
	// technique that was asked for.
	if (ImGui::BeginCombo("Technique", skinning.GetMethodName()))
	{
		for (SkinningController::Method option : kMethods)
		{
			if (ImGui::Selectable(SkinningController::GetMethodName(option), skinning.GetMethod() == option))
				skinning.SetMethod(option);
		}
		ImGui::EndCombo();
	}

	// Which weights that technique blends with: the rigid default, or bone heat
	// diffused on top of it.
	if (ImGui::BeginCombo("Weights", skinning.GetWeightModeName()))
	{
		for (SkinningController::WeightMode option : kWeightModes)
		{
			if (ImGui::Selectable(SkinningController::GetWeightModeName(option), skinning.GetWeightMode() == option))
				skinning.SetWeightMode(option);
		}
		ImGui::EndCombo();
	}

	if (skinning.GetWeightMode() == SkinningController::WeightMode::BoneHeat)
	{
		ImGui::SliderFloat("Alpha", &alpha, kMinAlpha, kMaxAlpha, "%.4f", ImGuiSliderFlags_Logarithmic);
		ImGui::SetItemTooltip("How hard the solve is pulled back to the closest-bone weights.\n"
		                      "Lower spreads the influence further over the surface.");
		if (ImGui::IsItemDeactivatedAfterEdit())
			skinning.SetBoneHeatAlpha(alpha);
		else if (!ImGui::IsItemActive())
			alpha = skinning.GetBoneHeatAlpha();
	}

	if (boundTo == nullptr)
	{
		ImGui::TextColored(kPendingColor, "Not bound - will animate with %s", skinning.GetMethodName());
		if (ImGui::Button("Bind"))
			skinning.BindMesh(mesh);
		return;
	}

	ImGui::TextColored(kActiveColor, "Animating with %s, %s weights",
	                   skinning.GetMethodName(), skinning.GetWeightModeName());

	bool showWeights = skinning.GetShowWeights();
	if (ImGui::Checkbox("Show weight influence", &showWeights))
		skinning.SetShowWeights(showWeights);

	if (ImGui::Button("Unbind"))
		skinning.UnbindMesh(*mesh);
}
