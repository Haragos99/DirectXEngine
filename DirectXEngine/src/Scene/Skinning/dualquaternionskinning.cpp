#include "dualquaternionskinning.h"

void DualQuaternionSkinning::Skin(const std::vector<DirectX::XMFLOAT4X4>& /*jointMatrices*/,
                                  std::vector<VertexData>& /*vertices*/) const
{
	// TODO: convert each joint matrix to a dual quaternion, blend, renormalise.
}
