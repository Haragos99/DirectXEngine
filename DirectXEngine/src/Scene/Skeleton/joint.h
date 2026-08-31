#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

// One node of the skeleton hierarchy. A joint knows its rest offset from its
// parent and the rotation the current pose applies to it; where it ends up in
// object space is derived from the whole chain by SkeletonPose.
class Joint
{
public:
	static constexpr int kNoParent = -1;

	Joint(std::string jointName, int jointId, int parent, const DirectX::XMFLOAT3& offset);

	const std::string& GetName() const { return name; }
	int GetId() const { return id; }
	int GetParentId() const { return parentId; }
	bool IsRoot() const { return parentId == kNoParent; }

	const std::vector<int>& GetChildren() const { return children; }
	void AddChild(int childId);

	const DirectX::XMFLOAT3& GetBindOffset() const { return bindOffset; }
	void SetBindOffset(const DirectX::XMFLOAT3& offset) { bindOffset = offset; }

	// Pose rotation around the joint origin, as pitch/yaw/roll in radians.
	const DirectX::XMFLOAT3& GetRotation() const { return rotation; }
	void SetRotation(const DirectX::XMFLOAT3& eulerRadians) { rotation = eulerRadians; }
	void ResetPose() { rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f); }

	// Placement inside the parent: rotate around the joint, then move out to
	// the bind offset, so a rotation carries every descendant with it.
	DirectX::XMMATRIX GetLocalMatrix() const;
	// Same placement with the pose ignored, i.e. the rest pose.
	DirectX::XMMATRIX GetBindLocalMatrix() const;

private:
	std::string name;
	int id = 0;
	int parentId = kNoParent;
	std::vector<int> children;
	DirectX::XMFLOAT3 bindOffset{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 rotation{ 0.0f, 0.0f, 0.0f };
};
