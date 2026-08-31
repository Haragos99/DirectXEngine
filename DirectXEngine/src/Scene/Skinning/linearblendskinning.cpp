#include "linearblendskinning.h"

void LinearBlendSkinning::Skin(const std::vector<DirectX::XMFLOAT4X4>& /*jointMatrices*/,
                               std::vector<VertexData>& /*vertices*/) const
{
	// TODO: sum weight[i] * jointMatrices[index[i]] per vertex, on the GPU.
}
