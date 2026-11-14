#include "skinbindingsection.h"

#include "skeleton.h"
#include "skinningregistry.h"
#include "uistate.h"

#include "imgui.h"

namespace
{
	const ImVec4 kActiveColor(0.45f, 0.90f, 0.45f, 1.0f);
	const ImVec4 kPendingColor(0.90f, 0.78f, 0.35f, 1.0f);
	const ImVec4 kWarningColor(0.95f, 0.45f, 0.35f, 1.0f);

	constexpr SkinWeightView kWeightViews[] = {
		SkinWeightView::Off,
		SkinWeightView::Blended,
		SkinWeightView::SingleInfluence,
	};

	ImVec4 ToImVec4(const DirectX::XMFLOAT3& color)
	{
		return ImVec4(color.x, color.y, color.z, 1.0f);
	}
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

std::shared_ptr<Skeleton> SkinBindingSection::FindBoundSkeleton(const std::vector<std::shared_ptr<Object3D>>& objects,
                                                                const Object3D& mesh)
{
	std::vector<std::shared_ptr<Skeleton>> rigs;
	CollectSkeletons(objects, rigs);
	for (const std::shared_ptr<Skeleton>& rig : rigs)
	{
		if (rig->GetSkinning().IsBound(mesh))
			return rig;
	}
	return nullptr;
}

void SkinBindingSection::DrawSolverParameters(SkinningController& skinning)
{
	const std::span<const SolverParameter> parameters = skinning.GetSolverParameters();
	if (parameterEdits.size() != parameters.size())
		parameterEdits.assign(parameters.size(), 0.0f);

	for (size_t i = 0; i < parameters.size(); ++i)
	{
		const SolverParameter& parameter = parameters[i];
		float& edit = parameterEdits[i];

		ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
		if (parameter.logarithmic)
			flags |= ImGuiSliderFlags_Logarithmic;

		ImGui::PushID(static_cast<int>(i));
		ImGui::SliderFloat(parameter.name, &edit, parameter.min, parameter.max, "%.4f", flags);
		if (parameter.tooltip != nullptr)
			ImGui::SetItemTooltip("%s", parameter.tooltip);
		// Solving is too costly to redo on every frame of a drag. Outside a drag
		// the edit follows the solver, so switching solvers or rigs shows its value.
		if (ImGui::IsItemDeactivatedAfterEdit())
			skinning.SetSolverParameter(i, edit);
		else if (!ImGui::IsItemActive())
			edit = parameter.value;
		ImGui::PopID();
	}
}

void SkinBindingSection::DrawBonePicker(SkinningController& skinning, const Object3D& mesh)
{
	const std::vector<SkinInfluence>* influences = skinning.GetInfluences(mesh);
	if (influences == nullptr || influences->empty())
		return;

	const SkinInfluence* focused = nullptr;
	for (const SkinInfluence& influence : *influences)
	{
		if (influence.bone == skinning.GetFocusedBone())
			focused = &influence;
	}

	// Nothing focused yet: show the first bone rather than an all-blue mesh.
	if (focused == nullptr)
	{
		focused = &influences->front();
		skinning.SetFocusedBone(focused->bone);
	}

	ImGui::ColorButton("##focused", ToImVec4(focused->color), ImGuiColorEditFlags_NoTooltip);
	ImGui::SameLine();
	if (ImGui::BeginCombo("Bone", focused->name.c_str()))
	{
		for (const SkinInfluence& influence : *influences)
		{
			ImGui::PushID(influence.bone);
			ImGui::ColorButton("##color", ToImVec4(influence.color), ImGuiColorEditFlags_NoTooltip);
			ImGui::SameLine();
			if (ImGui::Selectable(influence.name.c_str(), &influence == focused))
				skinning.SetFocusedBone(influence.bone);
			ImGui::PopID();
		}
		ImGui::EndCombo();
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
	const std::span<const SkinDeformerEntry> deformers = SkinningRegistry::Deformers();
	if (ImGui::BeginCombo("Technique", skinning.GetDeformerName()))
	{
		for (size_t i = 0; i < deformers.size(); ++i)
		{
			if (ImGui::Selectable(deformers[i].name, skinning.GetDeformerIndex() == i))
				skinning.SetDeformer(i);
		}
		ImGui::EndCombo();
	}

	// Which weights that technique blends with.
	const std::span<const WeightSolverEntry> solvers = SkinningRegistry::WeightSolvers();
	if (ImGui::BeginCombo("Weights", skinning.GetWeightSolverName()))
	{
		for (size_t i = 0; i < solvers.size(); ++i)
		{
			if (ImGui::Selectable(solvers[i].name, skinning.GetWeightSolverIndex() == i))
				skinning.SetWeightSolver(i);
		}
		ImGui::EndCombo();
	}

	DrawSolverParameters(skinning);

	if (boundTo == nullptr)
	{
		ImGui::TextColored(kPendingColor, "Not bound - will animate with %s", skinning.GetDeformerName());
		if (ImGui::Button("Bind"))
			skinning.BindMesh(mesh);
		return;
	}

	ImGui::TextColored(kActiveColor, "Animating with %s, %s weights",
	                   skinning.GetDeformerName(), skinning.GetWeightSolverName());

	// A solver that could not run leaves the closest-bone weights behind; say so
	// rather than let the label above claim otherwise.
	if (const std::optional<SkinningController::BindingView> binding = skinning.GetBinding(*mesh);
		binding && binding->status != WeightSolveStatus::Ok)
	{
		ImGui::TextColored(kWarningColor, "%s failed: %s.", skinning.GetWeightSolverName(),
		                   GetWeightSolveStatusMessage(binding->status));
		ImGui::TextColored(kWarningColor, "Using closest-bone weights instead.");
	}

	if (ImGui::BeginCombo("Weight view", SkinningController::GetWeightViewName(skinning.GetWeightView())))
	{
		for (SkinWeightView option : kWeightViews)
		{
			if (ImGui::Selectable(SkinningController::GetWeightViewName(option), skinning.GetWeightView() == option))
				skinning.SetWeightView(option);
		}
		ImGui::EndCombo();
	}
	ImGui::SetItemTooltip("Blended: every vertex in the sum of its bones' colours.\n"
	                      "Single bone: one bone's weight, blue 0 to red 1.");

	if (skinning.GetWeightView() == SkinWeightView::SingleInfluence)
		DrawBonePicker(skinning, *mesh);

	ImGui::Checkbox("Weight table", &state.skinWeightsWindowOpen);
	ImGui::SetItemTooltip("Every vertex's weight per bone, in a window of its own.");

	if (ImGui::Button("Unbind"))
		skinning.UnbindMesh(*mesh);
}
