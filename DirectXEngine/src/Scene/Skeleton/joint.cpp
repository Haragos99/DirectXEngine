#include "joint.h"

using namespace DirectX;

Joint::Joint(std::string jointName, int jointId, int parent, const XMFLOAT3& offset)
	: name(std::move(jointName)), id(jointId), parentId(parent), bindOffset(offset)
{
}

void Joint::AddChild(int childId)
{
	children.push_back(childId);
}

XMFLOAT3 Joint::GetTranslation() const
{
	return XMFLOAT3(bindOffset.x + poseTranslation.x,
	                bindOffset.y + poseTranslation.y,
	                bindOffset.z + poseTranslation.z);
}

XMMATRIX Joint::GetLocalMatrix() const
{
	const XMFLOAT3 translation = GetTranslation();
	return XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z)
		* XMMatrixTranslation(translation.x, translation.y, translation.z);
}

XMMATRIX Joint::GetBindLocalMatrix() const
{
	return XMMatrixTranslation(bindOffset.x, bindOffset.y, bindOffset.z);
}
