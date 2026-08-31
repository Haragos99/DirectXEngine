#pragma once
#include "iskinningtechnique.h"

// Dual quaternion skinning: blends rigid transforms instead of matrices, so
// twisted joints keep their volume. Costs more per vertex than LBS.
class DualQuaternionSkinning : public ISkinningTechnique
{
public:
	const char* GetName() const override { return "Dual Quaternion (DQS)"; }
	void Skin(const std::vector<DirectX::XMFLOAT4X4>& jointMatrices,
	          std::vector<VertexData>& vertices) const override;
};
