#pragma once
#include "computeskindeformer.h"

// GPU linear blend skinning: every vertex is the weighted sum of where each of
// its influences' matrices would put it.
class LinearBlendSkinning : public ComputeSkinDeformer
{
public:
	LinearBlendSkinning(Microsoft::WRL::ComPtr<ID3D11Device> _device,
	                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	const char* GetName() const override { return "Linear Blend"; }

protected:
	// The matrices go up as they are.
	const void* EncodePalette(const std::vector<DirectX::XMFLOAT4X4>& palette) override { return palette.data(); }
};
