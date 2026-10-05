#pragma once
#include "skinweights.h"
#include "vertex.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <vector>


class ISkinningTechnique
{
public:
	virtual ~ISkinningTechnique() = default;

	virtual const char* GetName() const = 0;

	// Uploads everything that stays constant while the mesh stays bound.
	virtual bool Prepare(const std::vector<VertexData>& restVertices,
	                     const std::vector<SkinWeights>& weights,
	                     int jointCount) = 0;

	// One matrix per joint, already expressed in the bound mesh's local space.
	virtual void Deform(const std::vector<DirectX::XMFLOAT4X4>& palette) = 0;

	// Optional: write the dominant joint into the texcoord so the weights can be
	// looked at. Techniques that cannot show them simply keep drawing normally.
	virtual void SetWeightDebug(bool /*enabled*/) {}

	// Vertex buffer holding the deformed result, or null before Prepare.
	virtual ID3D11Buffer* GetDeformedVertices() const = 0;
};
