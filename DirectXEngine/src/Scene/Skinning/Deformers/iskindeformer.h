#pragma once
#include "skinweights.h"
#include "vertex.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <vector>

// What the deformed vertices carry in their NORMAL slot for the weight view.
// The values are what SkinningCommon.hlsli expects in gViewMode.
enum class SkinWeightView : UINT
{
	Off = 0,             // the real normal
	Blended = 1,         // sum of weight * bone colour
	SingleInfluence = 2, // heat ramp of one influence's weight
};

// Per-frame stage of skinning: takes the weights solved at bind time and one
// matrix per influence, and produces the deformed vertex buffer the mesh draws.
// Correctives (Correctives/iskincorrective.h) run on that buffer afterwards.
class ISkinDeformer
{
public:
	virtual ~ISkinDeformer() = default;

	virtual const char* GetName() const = 0;

	// Uploads everything that stays constant while the mesh stays bound.
	// `influenceColors` has one entry per weight table column.
	virtual bool Prepare(const std::vector<VertexData>& restVertices,
	                     const SkinWeightTable& weights,
	                     const std::vector<DirectX::XMFLOAT3>& influenceColors) = 0;

	// One matrix per influence (weight table column), already expressed in the
	// bound mesh's local space.
	virtual void Deform(const std::vector<DirectX::XMFLOAT4X4>& palette) = 0;

	// Optional: write a weight colour instead of the normal so the weights can
	// be looked at. `selectedInfluence` is only used by SingleInfluence, -1 for
	// none. Deformers that cannot show them simply keep drawing normally.
	virtual void SetWeightView(SkinWeightView /*view*/, int /*selectedInfluence*/) {}

	// Vertex buffer holding the deformed result, or null before Prepare.
	virtual ID3D11Buffer* GetDeformedVertices() const = 0;
};
