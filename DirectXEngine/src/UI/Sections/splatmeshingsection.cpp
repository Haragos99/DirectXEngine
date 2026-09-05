#include "splatmeshingsection.h"
#include "splatcloudobject.h"
#include "splatdensityvolume.h"

#include "imgui.h"

SplatCloudObject* SplatMeshingSection::AsSplatCloud(const UIState& state)
{
	return dynamic_cast<SplatCloudObject*>(state.SelectedObject().get());
}

bool SplatMeshingSection::IsVisible(const UIState& state) const
{
	return AsSplatCloud(state) != nullptr;
}

void SplatMeshingSection::Draw(UIState& state)
{
	SplatCloudObject* cloud = AsSplatCloud(state);
	if (cloud == nullptr || !cloud->IsLoaded())
		return;

	ImGui::SliderInt("Grid resolution", &resolution, 16, 256);
	ImGui::SetItemTooltip("Cells along the longest axis. The main quality knob; cost grows cubically.");
	ImGui::SliderFloat("Density threshold", &isoLevel, 0.01f, 1.0f);
	ImGui::SetItemTooltip("Lower keeps faint splats and fills gaps, higher carves back to the dense core.");
	ImGui::SliderFloat("Splat reach", &reach, 1.0f, 4.0f, "%.1f sigma");
	ImGui::SetItemTooltip("How far each Gaussian spreads. Higher closes holes but rounds off detail.");

	ImGui::BeginDisabled(state.meshingActive);
	if (ImGui::Button("Create mesh from splats") && createIsoSurface)
	{
		createIsoSurface(std::make_shared<SplatDensityVolume>(
			                 cloud->GetSplats(),
			                 cloud->GetBoundsMin(),
			                 cloud->GetBoundsMax(),
			                 reach,
			                 cloud->GetTransform(),
			                 cloud->GetName()),
		                 resolution, isoLevel);
	}
	ImGui::EndDisabled();

	ImGui::TextDisabled("Voxelises the cloud, then meshes it with marching cubes.");
}
