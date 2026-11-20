#include "marchingcubessection.h"
#include "scalarfieldvolume.h"
#include "spherefield.h"

#include "imgui.h"

void MarchingCubesSection::Draw(UIState& state)
{
	ImGui::SliderFloat("Radius", &radius, 0.25f, 5.0f);
	ImGui::SliderInt("Resolution", &resolution, 8, 256);
	ImGui::SliderFloat("Iso level", &isoLevel, -1.0f, 1.0f);

	ImGui::BeginDisabled(state.meshingActive);
	if (ImGui::Button("Generate sphere mesh") && createIsoSurface)
	{
		auto sphere = std::make_shared<SphereField>(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), radius);
		createIsoSurface(std::make_shared<ScalarFieldVolume>(std::move(sphere)), resolution, isoLevel);
	}
	ImGui::EndDisabled();

	ImGui::TextDisabled("Samples the volume on a grid and meshes the iso surface.");
}
