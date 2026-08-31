#include "skeletonpose.h"

using namespace DirectX;

namespace
{
	const XMFLOAT4X4 kIdentity(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

void SkeletonPose::CaptureBindPose(const std::vector<Joint>& joints)
{
	inverseBind.assign(joints.size(), kIdentity);

	std::vector<XMFLOAT4X4> bind(joints.size(), kIdentity);
	for (const Joint& joint : joints)
	{
		XMMATRIX global = joint.GetBindLocalMatrix();
		if (!joint.IsRoot())
			global = global * XMLoadFloat4x4(&bind[joint.GetParentId()]);

		XMStoreFloat4x4(&bind[joint.GetId()], global);
		XMStoreFloat4x4(&inverseBind[joint.GetId()], XMMatrixInverse(nullptr, global));
	}
}

void SkeletonPose::Evaluate(const std::vector<Joint>& joints)
{
	globals.assign(joints.size(), kIdentity);
	skinning.assign(joints.size(), kIdentity);
	if (inverseBind.size() != joints.size())
		inverseBind.assign(joints.size(), kIdentity);

	for (const Joint& joint : joints)
	{
		XMMATRIX global = joint.GetLocalMatrix();
		if (!joint.IsRoot())
			global = global * XMLoadFloat4x4(&globals[joint.GetParentId()]);

		XMStoreFloat4x4(&globals[joint.GetId()], global);
		XMStoreFloat4x4(&skinning[joint.GetId()],
			XMLoadFloat4x4(&inverseBind[joint.GetId()]) * global);
	}
}

bool SkeletonPose::IsValidJoint(int jointId) const
{
	return jointId >= 0 && jointId < static_cast<int>(globals.size());
}

const XMFLOAT4X4& SkeletonPose::GetGlobalMatrix(int jointId) const
{
	return IsValidJoint(jointId) ? globals[jointId] : kIdentity;
}

XMFLOAT3 SkeletonPose::GetJointPosition(int jointId) const
{
	const XMFLOAT4X4& m = GetGlobalMatrix(jointId);
	return XMFLOAT3(m._41, m._42, m._43);
}
