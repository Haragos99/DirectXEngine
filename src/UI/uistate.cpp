#include "uistate.h"

size_t UIState::SceneObjectCount() const
{
	return sceneObjects != nullptr ? sceneObjects->size() : 0;
}

bool UIState::HasSelection() const
{
	return !selection.expired();
}

bool UIState::IsSelected(const std::shared_ptr<Object3D>& object) const
{
	return object != nullptr && object == selection.lock();
}

void UIState::Select(const std::shared_ptr<Object3D>& object)
{
	const std::shared_ptr<Object3D> previous = selection.lock();
	if (previous == object)
		return;

	if (previous)
		previous->OnSelected(false);

	selection = object;

	if (object)
		object->OnSelected(true);
}
