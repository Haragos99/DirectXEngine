#include "jointhandle.h"
#include "skeleton.h"

using namespace DirectX;

JointHandle::JointHandle(Skeleton& owner, int joint)
	: skeleton(owner), jointId(joint)
{
	name = owner.GetJoint(joint).GetName();
}

void JointHandle::SyncToJoint(FXMMATRIX skeletonWorld)
{
	setWorld(XMLoadFloat4x4(&skeleton.GetJointGlobalMatrix(jointId)) * skeletonWorld);
}

XMFLOAT3 JointHandle::GetPosition() const
{
	return skeleton.GetJointWorldPosition(jointId);
}

void JointHandle::AdjustPosition(float dx, float dy, float dz)
{
	skeleton.MoveJoint(jointId, XMFLOAT3(dx, dy, dz));
}

Transform JointHandle::GetTransform() const
{
	const Joint& joint = skeleton.GetJoint(jointId);

	Transform jointTransform;
	jointTransform.translation = joint.GetBindOffset();
	jointTransform.rotation = joint.GetRotation();
	return jointTransform;
}

void JointHandle::SetTransform(const Transform& newTransform)
{
	skeleton.SetJointOffset(jointId, newTransform.translation);
	skeleton.SetJointRotation(jointId, newTransform.rotation);
}

void JointHandle::OnSelected(bool selected)
{
	skeleton.SetSelectedJoint(selected ? jointId : -1);
}
