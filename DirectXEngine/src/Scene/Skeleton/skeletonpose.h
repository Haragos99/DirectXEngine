#pragma once
#include "joint.h"

#include <DirectXMath.h>
#include <vector>

// Turns the joint hierarchy into flat matrix arrays. It only knows how to walk
// the chain, so posing, drawing and (later) skinning all read the same result
// instead of each recomputing it.
//
// Joints must be ordered parent before child, which makes evaluation a single
// forward pass instead of a recursive walk.
class SkeletonPose
{
public:
	// Captures the rest pose. Call once, while every joint rotation is still identity.
	void CaptureBindPose(const std::vector<Joint>& joints);
	// Recomputes the global and skinning matrices from the current rotations.
	void Evaluate(const std::vector<Joint>& joints);

	size_t GetJointCount() const { return globals.size(); }
	bool IsValidJoint(int jointId) const;

	// Joint placement in object space.
	const DirectX::XMFLOAT4X4& GetGlobalMatrix(int jointId) const;
	DirectX::XMFLOAT3 GetJointPosition(int jointId) const;

	// global * inverseBind per joint: exactly the palette a skinning pass
	// uploads to the GPU.
	const std::vector<DirectX::XMFLOAT4X4>& GetSkinningMatrices() const { return skinning; }

private:
	std::vector<DirectX::XMFLOAT4X4> globals;
	std::vector<DirectX::XMFLOAT4X4> inverseBind;
	std::vector<DirectX::XMFLOAT4X4> skinning;
};
