#pragma once
#include <DirectXMath.h>

// A rigid link between two joints. The bone owns no transform of its own: its
// endpoints are whatever the current pose puts the two joints at, so posing a
// joint automatically moves every bone attached to it.
class Bone
{
public:
	Bone(int boneId, int start, int end, const DirectX::XMFLOAT3& boneColor);

	int GetId() const { return id; }
	int GetStartJoint() const { return startJoint; }
	int GetEndJoint() const { return endJoint; }
	const DirectX::XMFLOAT3& GetColor() const { return color; }

private:
	int id = 0;
	int startJoint = 0;
	int endJoint = 0;
	DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
};

// Distinct colours so neighbouring bones stay tellable apart in the viewport.
namespace BoneColors
{
	DirectX::XMFLOAT3 For(int boneIndex);
}
