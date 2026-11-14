#pragma once
#include "vertex.h"

#include <DirectXMath.h>
#include <vector>

class HalfEdgeMesh;

// One influence's bone in the rig's bind pose, in rig space. A root fallback
// influence (a rig without bones) has start == end.
struct BoneSegment
{
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
};

// Everything a weight solver, and later a corrective, may read about the mesh
// being bound. Built by SkinningController for every solve.
struct SkinBindContext
{
	const std::vector<VertexData>& restVertices; // mesh local space, as bound
	const HalfEdgeMesh* topology;                // welded connectivity, may be null
	DirectX::XMFLOAT4X4 meshBindToRig;           // mesh local -> rig space at bind time
	std::vector<BoneSegment> bones;              // one per influence, in column order
};
