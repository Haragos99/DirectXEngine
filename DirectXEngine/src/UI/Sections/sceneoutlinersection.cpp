#include "sceneoutlinersection.h"

#include "imgui.h"

namespace
{
	// Payload type of a hierarchy row drag. The dragged object itself is kept
	// on the section, so only a marker travels through ImGui.
	constexpr const char* kDragPayload = "OUTLINER_OBJECT";
}

void SceneOutlinerSection::Draw(UIState& state)
{
	if (state.SceneObjectCount() == 0)
	{
		ImGui::TextDisabled("No scene objects yet");
		return;
	}

	for (const std::shared_ptr<Object3D>& object : *state.sceneObjects)
		DrawNode(state, object);

	ImGui::Spacing();
	if (const std::shared_ptr<Object3D> selected = state.SelectedObject())
	{
		ImGui::Text("Selected: %s", selected->GetName().c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear"))
			state.Select(nullptr);
		ImGui::SameLine();
		if (ImGui::SmallButton("Unparent") && reparent && selected->GetParent())
			reparent(selected, nullptr);
	}
	else
	{
		ImGui::TextDisabled("No object selected. Drag a row onto another to parent it.");
	}
}

void SceneOutlinerSection::DrawNode(UIState& state, const std::shared_ptr<Object3D>& object)
{
	if (!object)
		return;

	const bool leaf = object->GetChildren().empty();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
	                         | ImGuiTreeNodeFlags_DefaultOpen
	                         | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (leaf)
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (state.IsSelected(object))
		flags |= ImGuiTreeNodeFlags_Selected;

	ImGui::PushID(object.get());
	const bool open = ImGui::TreeNodeEx("node", flags, "%s", object->GetName().c_str());

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		state.Select(object);

	DrawDragAndDrop(object);
	ImGui::PopID();

	if (!open || leaf)
		return;

	// Copied, because a drop handled below can reparent out of this list.
	const std::vector<std::shared_ptr<Object3D>> childrenSnapshot = object->GetChildren();
	for (const std::shared_ptr<Object3D>& child : childrenSnapshot)
		DrawNode(state, child);

	ImGui::TreePop();
}

void SceneOutlinerSection::DrawDragAndDrop(const std::shared_ptr<Object3D>& object)
{
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover))
	{
		dragged = object;
		ImGui::SetDragDropPayload(kDragPayload, nullptr, 0);
		ImGui::Text("%s", object->GetName().c_str());
		ImGui::EndDragDropSource();
	}

	if (!ImGui::BeginDragDropTarget())
		return;

	if (ImGui::AcceptDragDropPayload(kDragPayload) != nullptr)
	{
		if (const std::shared_ptr<Object3D> child = dragged.lock(); child && reparent)
			reparent(child, object);
		dragged.reset();
	}
	ImGui::EndDragDropTarget();
}
