#pragma once
#include "computeskindeformer.h"

// GPU dual quaternion skinning. Blends rigid transforms rather than matrices,
// so a twisted joint keeps its volume where linear blending pinches it.
//
// A dual quaternion carries no scale, so any scale in the palette is dropped
// when the matrices are converted.
class DualQuaternionSkinning : public ComputeSkinDeformer
{
public:
	DualQuaternionSkinning(Microsoft::WRL::ComPtr<ID3D11Device> _device,
	                       Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	const char* GetName() const override { return "Dual Quaternion"; }

protected:
	void OnPrepared(UINT influenceCount) override { dualQuaternions.resize(influenceCount); }
	const void* EncodePalette(const std::vector<DirectX::XMFLOAT4X4>& palette) override;

private:
	// Mirrors DualQuat in DualQuaternionSkinningCS.hlsl.
	struct DualQuaternion
	{
		DirectX::XMFLOAT4 real;
		DirectX::XMFLOAT4 dual;
	};

	// Rebuilt every frame; kept as a member so the upload allocates nothing.
	std::vector<DualQuaternion> dualQuaternions;
};
