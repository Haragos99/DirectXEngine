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

XMMATRIX Joint::GetLocalMatrix() const
{
	return XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z)
		* XMMatrixTranslation(bindOffset.x, bindOffset.y, bindOffset.z);
}

XMMATRIX Joint::GetBindLocalMatrix() const
{
	return XMMatrixTranslation(bindOffset.x, bindOffset.y, bindOffset.z);
}
