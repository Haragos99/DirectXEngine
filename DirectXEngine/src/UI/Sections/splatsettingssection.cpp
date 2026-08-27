#include "splatsettingssection.h"
#include "splatcloudobject.h"

#include "imgui.h"

namespace
{
	constexpr float kMinScale = 0.05f;
	constexpr float kMaxScale = 8.0f;
}

SplatCloudObject* SplatSettingsSection::AsSplatCloud(const UIState& state)
{
	return dynamic_cast<SplatCloudObject*>(state.SelectedObject().get());
}

bool SplatSettingsSection::IsVisible(const UIState& state) const
{
	return AsSplatCloud(state) != nullptr;
}

void SplatSettingsSection::Draw(UIState& state)
{
	SplatCloudObject* cloud = AsSplatCloud(state);
	if (cloud == nullptr)
		return;

	const char* primitive = cloud->GetPrimitiveName();
	ImGui::Text("%zu %s", cloud->GetSplatCount(), primitive);

	float scale = cloud->GetSplatScale();
	const std::string label = std::string(primitive) + " size";
	if (ImGui::SliderFloat(label.c_str(), &scale, kMinScale, kMaxScale, "%.2fx", ImGuiSliderFlags_Logarithmic))
		cloud->SetSplatScale(scale);

	ImGui::SameLine();
	if (ImGui::SmallButton("Reset"))
		cloud->SetSplatScale(1.0f);
}
