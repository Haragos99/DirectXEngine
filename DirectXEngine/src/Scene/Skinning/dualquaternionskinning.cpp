#include "dualquaternionskinning.h"

#include <d3dcompiler.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
	// Mirrors SkinConstants in DualQuaternionSkinningCS.hlsl.
	struct SkinConstants
	{
		UINT vertexCount;
		UINT jointCount;
		UINT weightColors;
		UINT padding;
	};

	constexpr UINT kThreadGroupSize = 64;
}

DualQuaternionSkinning::DualQuaternionSkinning(ComPtr<ID3D11Device> _device,
                                               ComPtr<ID3D11DeviceContext> _context)
	: device(_device), context(_context)
{
}

bool DualQuaternionSkinning::LoadShader()
{
	if (computeShader)
		return true;

	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errors;
	HRESULT hr = D3DCompileFromFile(L"shaders\\DualQuaternionSkinningCS.hlsl",
	                                nullptr, nullptr,
	                                "CSMain", "cs_5_0",
	                                D3DCOMPILE_ENABLE_STRICTNESS, 0,
	                                &blob, &errors);
	if (FAILED(hr))
		return false;

	return SUCCEEDED(device->CreateComputeShader(blob->GetBufferPointer(),
	                                             blob->GetBufferSize(),
	                                             nullptr, &computeShader));
}

bool DualQuaternionSkinning::CreateInput(const void* data, UINT stride, UINT count,
                                         ComPtr<ID3D11Buffer>& buffer,
                                         ComPtr<ID3D11ShaderResourceView>& view) const
{
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = stride * count;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = stride;

	D3D11_SUBRESOURCE_DATA initial = {};
	initial.pSysMem = data;

	if (FAILED(device->CreateBuffer(&desc, data ? &initial : nullptr, &buffer)))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	viewDesc.Buffer.NumElements = count;

	return SUCCEEDED(device->CreateShaderResourceView(buffer.Get(), &viewDesc, &view));
}

bool DualQuaternionSkinning::CreateOutput(const std::vector<VertexData>& restVertices)
{
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = static_cast<UINT>(restVertices.size() * sizeof(VertexData));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS;
	// Raw, because a structured buffer cannot also feed the input assembler.
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	D3D11_SUBRESOURCE_DATA initial = {};
	initial.pSysMem = restVertices.data();

	if (FAILED(device->CreateBuffer(&desc, &initial, &deformedVertices)))
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	viewDesc.Buffer.NumElements = desc.ByteWidth / 4;

	return SUCCEEDED(device->CreateUnorderedAccessView(deformedVertices.Get(), &viewDesc, &deformedView));
}

bool DualQuaternionSkinning::Prepare(const std::vector<VertexData>& restVertices,
                                     const SkinWeightTable& weights)
{
	restBuffer.Reset();
	restView.Reset();
	weightBuffer.Reset();
	weightView.Reset();
	paletteBuffer.Reset();
	paletteView.Reset();
	constantBuffer.Reset();
	deformedVertices.Reset();
	deformedView.Reset();
	dualQuaternions.clear();
	vertexCount = 0;
	jointCount = 0;

	if (restVertices.empty() || restVertices.size() != weights.VertexCount() || weights.JointCount() == 0)
		return false;

	if (!LoadShader())
		return false;

	const UINT count = static_cast<UINT>(restVertices.size());
	const UINT joints = static_cast<UINT>(weights.JointCount());
	// The table keeps readable per-vertex rows; the GPU needs one array.
	const std::vector<float> flatWeights = weights.Flatten();

	if (!CreateInput(restVertices.data(), sizeof(VertexData), count, restBuffer, restView))
		return false;
	if (!CreateInput(flatWeights.data(), sizeof(float), count * joints, weightBuffer, weightView))
		return false;
	if (!CreateInput(nullptr, sizeof(DualQuaternion), joints, paletteBuffer, paletteView))
		return false;
	if (!CreateOutput(restVertices))
		return false;

	D3D11_BUFFER_DESC constants = {};
	constants.Usage = D3D11_USAGE_DEFAULT;
	constants.ByteWidth = sizeof(SkinConstants);
	constants.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	if (FAILED(device->CreateBuffer(&constants, nullptr, &constantBuffer)))
		return false;

	vertexCount = count;
	jointCount = joints;
	dualQuaternions.resize(jointCount);

	UpdateConstants();
	return true;
}

void DualQuaternionSkinning::UpdateConstants()
{
	if (!constantBuffer)
		return;

	const SkinConstants values{ vertexCount, jointCount, weightDebug ? 1u : 0u, 0u };
	context->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &values, 0, 0);
}

void DualQuaternionSkinning::SetWeightDebug(bool enabled)
{
	if (weightDebug == enabled)
		return;

	weightDebug = enabled;
	UpdateConstants();
}

void DualQuaternionSkinning::Deform(const std::vector<XMFLOAT4X4>& palette)
{
	if (!computeShader || vertexCount == 0 || palette.size() != jointCount)
		return;

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

	// The output is a vertex buffer, so it must leave the input assembler
	// before it can be bound for writing.
	ID3D11Buffer* const noVertexBuffer[] = { nullptr };
	const UINT zero = 0;
	context->IASetVertexBuffers(0, 1, noVertexBuffer, &zero, &zero);

	context->UpdateSubresource(paletteBuffer.Get(), 0, nullptr, dualQuaternions.data(), 0, 0);

	ID3D11ShaderResourceView* const views[] = { restView.Get(), weightView.Get(), paletteView.Get() };
	ID3D11UnorderedAccessView* const targets[] = { deformedView.Get() };
	ID3D11Buffer* const constants[] = { constantBuffer.Get() };

	context->CSSetShader(computeShader.Get(), nullptr, 0);
	context->CSSetShaderResources(0, 3, views);
	context->CSSetUnorderedAccessViews(0, 1, targets, nullptr);
	context->CSSetConstantBuffers(0, 1, constants);

	context->Dispatch((vertexCount + kThreadGroupSize - 1) / kThreadGroupSize, 1, 1);

	ID3D11ShaderResourceView* const noViews[] = { nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* const noTargets[] = { nullptr };
	context->CSSetShaderResources(0, 3, noViews);
	context->CSSetUnorderedAccessViews(0, 1, noTargets, nullptr);
	context->CSSetShader(nullptr, nullptr, 0);
}
