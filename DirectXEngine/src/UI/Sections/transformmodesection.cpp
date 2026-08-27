#include "transformmodesection.h"

#include "imgui.h"

void TransformModeSection::Draw(UIState& state)
{
	int mode = static_cast<int>(state.gizmoMode);
	ImGui::RadioButton("Move", &mode, static_cast<int>(GizmoMode::Move));
	ImGui::SameLine();
	ImGui::RadioButton("Scale", &mode, static_cast<int>(GizmoMode::Scale));
	state.gizmoMode = static_cast<GizmoMode>(mode);

	ImGui::TextDisabled(state.gizmoMode == GizmoMode::Move
		? "Arrows: move on one axis - centre cube: move freely"
		: "Cubes: scale on one axis - centre cube: scale uniformly");
}
