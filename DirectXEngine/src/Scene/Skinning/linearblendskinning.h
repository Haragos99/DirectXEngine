#pragma once
#include "iskinningtechnique.h"

// Linear blend skinning: weighted sum of the joint matrices. Cheap, and the
// default everywhere, at the price of collapsing volume on twisted joints.
class LinearBlendSkinning : public ISkinningTechnique
{
public:
	const char* GetName() const override { return "Linear Blend (LBS)"; }
	void Skin(const std::vector<DirectX::XMFLOAT4X4>& jointMatrices,
	          std::vector<VertexData>& vertices) const override;
};
