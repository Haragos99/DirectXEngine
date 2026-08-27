#include "sceneoutlinersection.h"

#include "imgui.h"

#include <string>

namespace
{
	std::string LabelOf(const std::shared_ptr<Object3D>& object, size_t index)
	{
		const std::string name = object ? object->GetName() : "<null>";
		return name + " #" + std::to_string(index);
	}
}

void SceneOutlinerSection::Draw(UIState& state)
{
	const size_t count = state.SceneObjectCount();

	if (ImGui::BeginChild("SceneObjectList", ImVec2(0.0f, 140.0f), true))
	{
		if (count == 0)
		{
			ImGui::TextDisabled("No scene objects yet");
		}
		else
		{
			for (size_t i = 0; i < count; ++i)
			{
				const std::string label = LabelOf((*state.sceneObjects)[i], i);
				const bool isSelected = (state.selectedIndex == static_cast<int>(i));
				if (ImGui::Selectable(label.c_str(), isSelected))
					state.selectedIndex = static_cast<int>(i);
			}
		}
		ImGui::EndChild();
	}

	if (const std::shared_ptr<Object3D> selected = state.SelectedObject())
	{
		ImGui::Text("Selected: %s", selected->GetName().c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear selection"))
			state.selectedIndex = -1;
	}
	else
	{
		ImGui::TextDisabled("No object selected");
	}
}
