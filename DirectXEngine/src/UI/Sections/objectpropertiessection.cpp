#include "objectpropertiessection.h"

#include "imgui.h"

using namespace DirectX;

namespace
{
	constexpr float kMinScale = 0.001f;
	constexpr float kMaxScale = 1000.0f;

	float ClampScale(float value)
	{
		return value < kMinScale ? kMinScale : value;
	}
}

bool ObjectPropertiesSection::IsVisible(const UIState& state) const
{
	return state.SelectedObject() != nullptr;
}

void ObjectPropertiesSection::Draw(UIState& state)
{
	const std::shared_ptr<Object3D> object = state.SelectedObject();
	if (!object)
		return;

	ImGui::Text("Name: %s", object->GetName().c_str());
	ImGui::Spacing();

	Transform transform = object->GetTransform();
	bool changed = false;

	float translation[3] = { transform.translation.x, transform.translation.y, transform.translation.z };
	if (ImGui::DragFloat3("Position", translation, 0.05f, 0.0f, 0.0f, "%.3f"))
	{
		transform.translation = XMFLOAT3(translation[0], translation[1], translation[2]);
		changed = true;
	}

	// The transform stores radians, but degrees are what the user expects to edit.
	float rotation[3] = { XMConvertToDegrees(transform.rotation.x),
						  XMConvertToDegrees(transform.rotation.y),
						  XMConvertToDegrees(transform.rotation.z) };
	if (ImGui::DragFloat3("Rotation", rotation, 0.5f, 0.0f, 0.0f, "%.2f deg"))
	{
		transform.rotation = XMFLOAT3(XMConvertToRadians(rotation[0]),
									  XMConvertToRadians(rotation[1]),
									  XMConvertToRadians(rotation[2]));
		changed = true;
	}

	if (uniformScale)
	{
		float scale = transform.scale.x;
		if (ImGui::DragFloat("Scale", &scale, 0.01f, kMinScale, kMaxScale, "%.3f"))
		{
			const float clamped = ClampScale(scale);
			transform.scale = XMFLOAT3(clamped, clamped, clamped);
			changed = true;
		}
	}
	else
	{
		float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };
		if (ImGui::DragFloat3("Scale", scale, 0.01f, kMinScale, kMaxScale, "%.3f"))
		{
			transform.scale = XMFLOAT3(ClampScale(scale[0]), ClampScale(scale[1]), ClampScale(scale[2]));
			changed = true;
		}
	}

	ImGui::Checkbox("Uniform scale", &uniformScale);

	if (changed)
		object->SetTransform(transform);

	ImGui::Spacing();
	if (ImGui::Button("Reset transform"))
		object->SetTransform(Transform());

	ImGui::TextDisabled("Drag a field or ctrl+click it to type a value.");
}
