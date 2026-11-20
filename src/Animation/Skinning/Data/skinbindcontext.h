#pragma once
#include "vertex.h"

#include <DirectXMath.h>
#include <vector>

class HalfEdgeMesh;

struct BoneSegment
{
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
};

struct SkinBindContext
{
	const std::vector<VertexData>& restVertices; // mesh local space, as bound
	const HalfEdgeMesh* topology;
	DirectX::XMFLOAT4X4 meshBindToRig;
	std::vector<BoneSegment> bones;
};
