#include "linearblendskinning.h"

using Microsoft::WRL::ComPtr;

LinearBlendSkinning::LinearBlendSkinning(ComPtr<ID3D11Device> _device,
                                         ComPtr<ID3D11DeviceContext> _context)
	: ComputeSkinDeformer(_device, _context, L"shaders\\LinearBlendSkinningCS.hlsl",
	                      sizeof(DirectX::XMFLOAT4X4))
{
}
