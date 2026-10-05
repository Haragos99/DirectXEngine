#include "skinbindingsection.h"

#include "skeleton.h"
#include "uistate.h"

#include "imgui.h"

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

	if (ImGui::BeginCombo("Skeleton", rigs[selectedRig]->GetName().c_str()))
	{
		for (int i = 0; i < static_cast<int>(rigs.size()); ++i)
		{
			if (ImGui::Selectable(rigs[i]->GetName().c_str(), i == selectedRig))
				selectedRig = i;
		}
		ImGui::EndCombo();
	}

	// A mesh belongs to at most one rig, so the bound one is what we act on.
	Skeleton* boundTo = nullptr;
	for (const std::shared_ptr<Skeleton>& rig : rigs)
	{
		if (rig->GetSkinning().IsBound(*mesh))
			boundTo = rig.get();
	}

	if (boundTo == nullptr)
	{
		if (ImGui::Button("Bind"))
			rigs[selectedRig]->GetSkinning().BindMesh(mesh);
		return;
	}

	ImGui::Text("Bound to %s (%s)", boundTo->GetName().c_str(),
	            boundTo->GetSkinning().GetMethodName());

	bool showWeights = boundTo->GetSkinning().GetShowWeights();
	if (ImGui::Checkbox("Show weight influence", &showWeights))
		boundTo->GetSkinning().SetShowWeights(showWeights);

	if (ImGui::Button("Unbind"))
		boundTo->GetSkinning().UnbindMesh(*mesh);
}
