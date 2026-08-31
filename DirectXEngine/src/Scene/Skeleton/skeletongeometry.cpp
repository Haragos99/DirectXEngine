#include "skeletongeometry.h"

#include <algorithm>

using namespace DirectX;

namespace
{
	// Proportions of one bone rombus, as fractions of the bone length.
	constexpr float kRingOffset = 0.18f; // where the widest ring sits
	constexpr float kRingRadius = 0.11f;
	constexpr float kMinRadius = 0.012f;
	constexpr float kMaxRadius = 0.06f;
	constexpr float kJointHalfExtent = 0.022f;
	constexpr float kMinBoneLength = 1e-4f;

	const XMFLOAT3 kJointColor(0.85f, 0.85f, 0.85f);
	const XMFLOAT3 kHighlightColor(1.0f, 0.78f, 0.15f);

	XMFLOAT3 ToFloat3(FXMVECTOR v)
	{
		XMFLOAT3 out;
		XMStoreFloat3(&out, v);
		return out;
	}
}

void SkeletonGeometry::Build(const std::vector<Joint>& joints,
                             const std::vector<Bone>& bones,
                             const SkeletonPose& pose,
                             int highlightedJoint)
{
	vertices.clear();
	indices.clear();

	for (const Bone& bone : bones)
	{
		const bool highlighted = bone.GetStartJoint() == highlightedJoint
		                      || bone.GetEndJoint() == highlightedJoint;
		AppendBone(bone, pose, highlighted);
	}

	for (const Joint& joint : joints)
	{
		AppendJointMarker(pose.GetJointPosition(joint.GetId()),
		                  joint.GetId() == highlightedJoint);
	}
}

// Octahedron pointing from the start joint to the end joint: apex, a square
// ring near the start, and a tip at the end.
void SkeletonGeometry::AppendBone(const Bone& bone, const SkeletonPose& pose, bool highlighted)
{
	const XMFLOAT3 startF = pose.GetJointPosition(bone.GetStartJoint());
	const XMFLOAT3 endF = pose.GetJointPosition(bone.GetEndJoint());
	const XMVECTOR start = XMLoadFloat3(&startF);
	const XMVECTOR end = XMLoadFloat3(&endF);

	const XMVECTOR delta = XMVectorSubtract(end, start);
	const float length = XMVectorGetX(XMVector3Length(delta));
	if (length < kMinBoneLength)
		return; // degenerate link, nothing to draw

	const XMVECTOR axis = XMVectorScale(delta, 1.0f / length);
	// Any vector not parallel to the axis gives a usable side direction.
	const XMVECTOR reference = (fabsf(XMVectorGetY(axis)) < 0.9f)
		? XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
		: XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	const XMVECTOR u = XMVector3Normalize(XMVector3Cross(reference, axis));
	const XMVECTOR v = XMVector3Cross(axis, u);

	const float radius = std::clamp(length * kRingRadius, kMinRadius, kMaxRadius);
	const XMVECTOR ringCenter = XMVectorAdd(start, XMVectorScale(axis, length * kRingOffset));
	const XMFLOAT3 color = highlighted ? kHighlightColor : bone.GetColor();

	const UINT base = static_cast<UINT>(vertices.size());
	AppendVertex(startF, color); // 0: apex
	AppendVertex(endF, color);   // 1: tip
	for (int i = 0; i < 4; ++i)
	{
		// Ring corners at 0, 90, 180 and 270 degrees around the bone axis.
		const XMVECTOR side = (i % 2 == 0) ? u : v;
		const float sign = (i < 2) ? 1.0f : -1.0f;
		AppendVertex(ToFloat3(XMVectorAdd(ringCenter, XMVectorScale(side, radius * sign))), color);
	}

	// Ring corners live at offsets 2..5, ordered +u, +v, -u, -v around the axis.
	for (UINT i = 0; i < 4; ++i)
	{
		const UINT current = base + 2 + i;
		const UINT next = base + 2 + (i + 1) % 4;

		indices.push_back(base + 0); indices.push_back(current); indices.push_back(next);
		indices.push_back(base + 1); indices.push_back(next); indices.push_back(current);
	}
}

void SkeletonGeometry::AppendJointMarker(const XMFLOAT3& position, bool highlighted)
{
	static constexpr float signX[8] = { -1, 1, 1, -1, -1, 1, 1, -1 };
	static constexpr float signY[8] = { -1, -1, 1, 1, -1, -1, 1, 1 };
	static constexpr float signZ[8] = { -1, -1, -1, -1, 1, 1, 1, 1 };
	static constexpr UINT triangles[36] =
	{
		0, 1, 2,  0, 2, 3,
		4, 6, 5,  4, 7, 6,
		0, 4, 5,  0, 5, 1,
		3, 2, 6,  3, 6, 7,
		0, 3, 7,  0, 7, 4,
		1, 5, 6,  1, 6, 2
	};

	const XMFLOAT3 color = highlighted ? kHighlightColor : kJointColor;
	const UINT base = static_cast<UINT>(vertices.size());

	for (int corner = 0; corner < 8; ++corner)
	{
		AppendVertex(XMFLOAT3(position.x + signX[corner] * kJointHalfExtent,
		                      position.y + signY[corner] * kJointHalfExtent,
		                      position.z + signZ[corner] * kJointHalfExtent), color);
	}

	for (UINT corner : triangles)
		indices.push_back(base + corner);
}

void SkeletonGeometry::AppendVertex(const XMFLOAT3& position, const XMFLOAT3& color)
{
	// The skeleton shaders read the per-vertex colour out of the normal slot,
	// the same trick the gizmos use to stay on the shared input layout.
	VertexData vertex;
	vertex.position = position;
	vertex.normal = color;
	vertex.texcoord = XMFLOAT2(0.0f, 0.0f);
	vertices.push_back(vertex);
}
