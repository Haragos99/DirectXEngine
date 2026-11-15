#include "dualquaternionskinning.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

DualQuaternionSkinning::DualQuaternionSkinning(ComPtr<ID3D11Device> _device,
                                               ComPtr<ID3D11DeviceContext> _context)
	: ComputeSkinDeformer(_device, _context, L"shaders\\DualQuaternionSkinningCS.hlsl",
	                      sizeof(DualQuaternion))
{
}

const void* DualQuaternionSkinning::EncodePalette(const std::vector<XMFLOAT4X4>& palette)
{
	for (size_t i = 0; i < palette.size(); ++i)
	{
		XMMATRIX matrix = XMLoadFloat4x4(&palette[i]);
		const XMVECTOR translation = matrix.r[3];

		// Only the rigid part survives: renormalising the basis drops the scale
		// a dual quaternion could not represent anyway, and keeps the quaternion
		// extraction well conditioned.
		matrix.r[0] = XMVector3Normalize(matrix.r[0]);
		matrix.r[1] = XMVector3Normalize(matrix.r[1]);
		matrix.r[2] = XMVector3Normalize(matrix.r[2]);
		matrix.r[3] = g_XMIdentityR3;

		const XMVECTOR rotation = XMQuaternionNormalize(XMQuaternionRotationMatrix(matrix));

		// dual = 0.5 * (0, t) * rotation. XMQuaternionMultiply(a, b) evaluates
		// b * a, so the translation has to be passed second.
		const XMVECTOR offset = XMVectorSetW(translation, 0.0f);
		const XMVECTOR dual = XMVectorScale(XMQuaternionMultiply(rotation, offset), 0.5f);

		XMStoreFloat4(&dualQuaternions[i].real, rotation);
		XMStoreFloat4(&dualQuaternions[i].dual, dual);
	}
	return dualQuaternions.data();
}
