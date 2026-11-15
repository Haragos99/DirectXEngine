#include "computeskindeformer.h"

#include <d3dcompiler.h>
#include <windows.h>


using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
	// Mirrors SkinConstants in SkinningCommon.hlsli.
	struct SkinConstants
	{
		UINT vertexCount;
		UINT influenceCount;
		UINT viewMode;
		INT selectedInfluence;
	};
	static_assert(sizeof(SkinConstants) == 16, "constant buffers are 16 byte aligned");

	constexpr UINT kThreadGroupSize = 64;
	constexpr UINT kInputCount = 4;
}

ComputeSkinDeformer::ComputeSkinDeformer(ComPtr<ID3D11Device> _device,
                                         ComPtr<ID3D11DeviceContext> _context,
                                         std::wstring _shaderPath,
                                         UINT _paletteStride)
	: device(_device), context(_context), shaderPath(std::move(_shaderPath)), paletteStride(_paletteStride)
{
}

bool ComputeSkinDeformer::LoadShader()
{
	if (computeShader)
	{
		return true;
	}

	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errors;
	const HRESULT hr = D3DCompileFromFile(shaderPath.c_str(),
	                                      nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
	                                      "CSMain", "cs_5_0",
	                                      D3DCOMPILE_ENABLE_STRICTNESS, 0,
	                                      &blob, &errors);
	if (FAILED(hr))
	{
		if (errors)
		{
			OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
		}
		return false;
	}

	return SUCCEEDED(device->CreateComputeShader(blob->GetBufferPointer(),
	                                             blob->GetBufferSize(),
	                                             nullptr, &computeShader));
}

bool ComputeSkinDeformer::CreateInput(const void* data, UINT stride, UINT count,
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
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	viewDesc.Buffer.NumElements = count;

	return SUCCEEDED(device->CreateShaderResourceView(buffer.Get(), &viewDesc, &view));
}

bool ComputeSkinDeformer::CreateOutput(const std::vector<VertexData>& restVertices)
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
	{
		return false;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	viewDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	viewDesc.Buffer.NumElements = desc.ByteWidth / 4;

	return SUCCEEDED(device->CreateUnorderedAccessView(deformedVertices.Get(), &viewDesc, &deformedView));
}

void ComputeSkinDeformer::Release()
{
	restBuffer.Reset();
	restView.Reset();
	weightBuffer.Reset();
	weightView.Reset();
	paletteBuffer.Reset();
	paletteView.Reset();
	colorBuffer.Reset();
	colorView.Reset();
	constantBuffer.Reset();
	deformedVertices.Reset();
	deformedView.Reset();
	vertexCount = 0;
	influenceCount = 0;
}

bool ComputeSkinDeformer::Prepare(const std::vector<VertexData>& restVertices,
                                  const SkinWeightTable& weights,
                                  const std::vector<XMFLOAT3>& influenceColors)
{
	Release();

	if (restVertices.empty() || restVertices.size() != weights.VertexCount()
		|| weights.InfluenceCount() == 0 || influenceColors.size() != weights.InfluenceCount())
	{
		return false;
	}

	if (!LoadShader())
	{
		return false;
	}

	const UINT count = static_cast<UINT>(restVertices.size());
	const UINT influences = static_cast<UINT>(weights.InfluenceCount());

	if (!CreateInput(restVertices.data(), sizeof(VertexData), count, restBuffer, restView)
		|| !CreateInput(weights.Data().data(), sizeof(float), count * influences, weightBuffer, weightView)
		|| !CreateInput(nullptr, paletteStride, influences, paletteBuffer, paletteView)
		|| !CreateInput(influenceColors.data(), sizeof(XMFLOAT3), influences, colorBuffer, colorView)
		|| !CreateOutput(restVertices))
	{
		Release();
		return false;
	}

	D3D11_BUFFER_DESC constants = {};
	constants.Usage = D3D11_USAGE_DEFAULT;
	constants.ByteWidth = sizeof(SkinConstants);
	constants.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	if (FAILED(device->CreateBuffer(&constants, nullptr, &constantBuffer)))
	{
		Release();
		return false;
	}

	vertexCount = count;
	influenceCount = influences;
	OnPrepared(influenceCount);

	UpdateConstants();
	return true;
}

void ComputeSkinDeformer::UpdateConstants()
{
	if (!constantBuffer)
	{
		return;
	}

	const SkinConstants values{ vertexCount, influenceCount,
	                            static_cast<UINT>(weightViewMode), selectedInfluence };
	context->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &values, 0, 0);
}

void ComputeSkinDeformer::SetWeightView(SkinWeightView view, int selected)
{
	if (weightViewMode == view && selectedInfluence == selected)
	{
		return;
	}

	weightViewMode = view;
	selectedInfluence = selected;
	UpdateConstants();
}

void ComputeSkinDeformer::Deform(const std::vector<XMFLOAT4X4>& palette)
{
	if (!computeShader || vertexCount == 0 || palette.size() != influenceCount)
	{
		return;
	}

	context->UpdateSubresource(paletteBuffer.Get(), 0, nullptr, EncodePalette(palette), 0, 0);

	// The output is a vertex buffer, so it must leave the input assembler
	// before it can be bound for writing.
	ID3D11Buffer* const noVertexBuffer[] = { nullptr };
	const UINT zero = 0;
	context->IASetVertexBuffers(0, 1, noVertexBuffer, &zero, &zero);

	ID3D11ShaderResourceView* const views[kInputCount] = {
		restView.Get(), weightView.Get(), paletteView.Get(), colorView.Get() };
	ID3D11UnorderedAccessView* const targets[] = { deformedView.Get() };
	ID3D11Buffer* const constants[] = { constantBuffer.Get() };

	context->CSSetShader(computeShader.Get(), nullptr, 0);
	context->CSSetShaderResources(0, kInputCount, views);
	context->CSSetUnorderedAccessViews(0, 1, targets, nullptr);
	context->CSSetConstantBuffers(0, 1, constants);

	context->Dispatch((vertexCount + kThreadGroupSize - 1) / kThreadGroupSize, 1, 1);

	ID3D11ShaderResourceView* const noViews[kInputCount] = {};
	ID3D11UnorderedAccessView* const noTargets[] = { nullptr };
	context->CSSetShaderResources(0, kInputCount, noViews);
	context->CSSetUnorderedAccessViews(0, 1, noTargets, nullptr);
	context->CSSetShader(nullptr, nullptr, 0);
}
