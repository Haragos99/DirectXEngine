#include "viewportsection.h"

#include "imgui.h"

void ViewportSection::Draw(UIState& state)
{
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("Frame time: %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	if (ImGui::Button("Cycle render mode"))
		state.renderModeChangeRequested = true;

	ImGui::ColorEdit3("Clear color", state.clearColor);
	ImGui::Checkbox("Show ImGui demo window", &showDemoWindow);

	if (showDemoWindow)
		ImGui::ShowDemoWindow(&showDemoWindow);
}
