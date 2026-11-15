#pragma once
#include "../skinweights.h"
#include "vertex.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <vector>

enum class SkinWeightView : UINT
{
	Off = 0,
	Blended = 1,
	SingleInfluence = 2,
};

class ISkinDeformer
{
public:
	virtual ~ISkinDeformer() = default;

	virtual const char* GetName() const = 0;

	// Uploads everything that stays constant while the mesh stays bound.
	virtual bool Prepare(const std::vector<VertexData>& restVertices,
	                     const SkinWeightTable& weights,
	                     const std::vector<DirectX::XMFLOAT3>& influenceColors) = 0;

	virtual void Deform(const std::vector<DirectX::XMFLOAT4X4>& palette) = 0;

	virtual void SetWeightView(SkinWeightView /*view*/, int /*selectedInfluence*/) {}

	// Vertex buffer holding the deformed result, or null before Prepare.
	virtual ID3D11Buffer* GetDeformedVertices() const = 0;
};
