#pragma once
#include <DirectXMath.h>
#include <string>

// One column of a skin weight table: something a vertex can be bound to.
// Weights are per bone, as in the reference implementation. A bone is moved
// rigidly by its start joint's frame (rotating a joint turns what lies below
// it, not the bone leading into it), so that is the joint that drives it.
struct SkinInfluence
{
	static constexpr int kRootFallback = -1;

	int bone = kRootFallback;      // index into Skeleton::GetBones(), or kRootFallback
	int drivingJoint = 0;          // Bone::GetStartJoint(), or the root
	DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f }; // Bone::GetColor(): the only colour source
	std::string name;              // shown as the weight table's column header
};
