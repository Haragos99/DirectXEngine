#include "uistate.h"

size_t UIState::SceneObjectCount() const
{
	return sceneObjects != nullptr ? sceneObjects->size() : 0;
}

bool UIState::HasSelection() const
{
	return selectedIndex >= 0 && selectedIndex < static_cast<int>(SceneObjectCount());
}

std::shared_ptr<Object3D> UIState::SelectedObject() const
{
	return HasSelection() ? (*sceneObjects)[static_cast<size_t>(selectedIndex)] : nullptr;
}

void UIState::ValidateSelection()
{
	if (selectedIndex >= static_cast<int>(SceneObjectCount()))
		selectedIndex = -1;
}
