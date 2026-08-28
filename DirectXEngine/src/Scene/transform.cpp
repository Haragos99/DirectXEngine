#include "transform.h"

using namespace DirectX;

XMMATRIX Transform::ToMatrix() const
{
	return XMMatrixScaling(scale.x, scale.y, scale.z) *
		XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
		XMMatrixTranslation(translation.x, translation.y, translation.z);
}
