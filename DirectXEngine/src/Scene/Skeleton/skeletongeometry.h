#pragma once
#include "bone.h"
#include "joint.h"
#include "skeletonpose.h"
#include "vertex.h"

#include <d3d11.h>
#include <vector>

// Builds the drawable mesh of a posed skeleton: an octahedral "rombus" per bone
// plus a small marker box per joint. Kept apart from Skeleton so the look of the
// rig can change without touching the hierarchy or the render setup.
class SkeletonGeometry
{
public:
	// Rebuilds the mesh for the given pose. `highlightedJoint` is drawn in the
	// selection colour together with the bones that touch it; pass -1 for none.
	void Build(const std::vector<Joint>& joints,
	           const std::vector<Bone>& bones,
	           const SkeletonPose& pose,
	           int highlightedJoint);

	const std::vector<VertexData>& GetVertices() const { return vertices; }
	const std::vector<UINT>& GetIndices() const { return indices; }

private:
	void AppendBone(const Bone& bone, const SkeletonPose& pose, bool highlighted);
	void AppendJointMarker(const DirectX::XMFLOAT3& position, bool highlighted);
	void AppendVertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& color);

	std::vector<VertexData> vertices;
	std::vector<UINT> indices;
};
