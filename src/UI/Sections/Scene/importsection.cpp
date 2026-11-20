#include "importsection.h"

#include "imgui.h"

void ImportSection::Draw(UIState&)
{
	if (ImGui::Button("Import model"))
	{
		std::wstring path = dialog.Show();
		if (!path.empty())
		{
			lastImportedPath = std::move(path);
			if (importModel)
				importModel(lastImportedPath);
		}
	}

	ImGui::SameLine();
	if (lastImportedPath.empty())
	{
		ImGui::TextDisabled("No file selected");
	}
	else
	{
		ImGui::TextDisabled("Loaded");
		ImGui::TextWrapped("Path: %ls", lastImportedPath.c_str());
	}
}
