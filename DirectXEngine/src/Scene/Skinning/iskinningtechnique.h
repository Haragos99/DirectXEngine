#pragma once
#include "vertex.h"

#include <DirectXMath.h>
#include <vector>

// A way of deforming vertices from a joint matrix palette. Linear blend and
// dual quaternion skinning differ only in how they blend those matrices, so the
// rig picks a technique and never learns which one it got.
//
// TODO: no technique deforms anything yet. See Docs/skinning-compute-plan.md
// for the compute shader path these will run on.
class ISkinningTechnique
{
public:
	virtual ~ISkinningTechnique() = default;

	// Shown in the UI when picking a technique.
	virtual const char* GetName() const = 0;

	// Deforms `vertices` in place, one matrix per joint in `jointMatrices`.
	virtual void Skin(const std::vector<DirectX::XMFLOAT4X4>& jointMatrices,
	                  std::vector<VertexData>& vertices) const = 0;
};
