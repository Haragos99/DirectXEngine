#include "gaussiansplatshader.h"

#include <d3dcompiler.h>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

GaussianSplatShader::GaussianSplatShader(ComPtr<ID3D11Device> _device, ComPtr<ID3D11DeviceContext> _context)
	: Shader(_device, _context)
{
}

void GaussianSplatShader::CreateQuadGeometry()
{
	const std::vector<SplatCorner> corners =
	{
		{ DirectX::XMFLOAT2(-1.0f, -1.0f) },
		{ DirectX::XMFLOAT2( 1.0f, -1.0f) },
		{ DirectX::XMFLOAT2( 1.0f,  1.0f) },
		{ DirectX::XMFLOAT2(-1.0f,  1.0f) }
	};

	createVertexBuffer(corners);
	createInexxBuffer({ 0, 1, 2, 0, 2, 3 });
}

bool GaussianSplatShader::CreateSplatBuffer(const std::vector<SplatInstance>& splats)
{
	splatBuffer.Reset();
	splatSRV.Reset();
	if (splats.empty())
		return false;

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = static_cast<UINT>(splats.size() * sizeof(SplatInstance));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(SplatInstance);

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = splats.data();

	if (FAILED(device->CreateBuffer(&desc, &initData, &splatBuffer)))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	viewDesc.Buffer.FirstElement = 0;
	viewDesc.Buffer.NumElements = static_cast<UINT>(splats.size());

	return SUCCEEDED(device->CreateShaderResourceView(splatBuffer.Get(), &viewDesc, &splatSRV));
}

bool GaussianSplatShader::CreateOrderBuffer(size_t splatCount)
{
	orderBuffer.Reset();
	if (splatCount == 0)
		return false;

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = static_cast<UINT>(splatCount * sizeof(std::uint32_t));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	return SUCCEEDED(device->CreateBuffer(&desc, nullptr, &orderBuffer));
}

void GaussianSplatShader::UpdateOrder(const std::vector<std::uint32_t>& order)
{
	if (!orderBuffer || order.empty())
		return;

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (SUCCEEDED(context->Map(orderBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		memcpy(mapped.pData, order.data(), order.size() * sizeof(std::uint32_t));
		context->Unmap(orderBuffer.Get(), 0);
	}
}

void GaussianSplatShader::CreateFrameBuffer()
{
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(SplatFrameBuffer);
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(device->CreateBuffer(&desc, nullptr, &frameBuffer)))
		throw std::runtime_error("Failed to create the Gaussian splat frame buffer");
}

void GaussianSplatShader::CreateBlendState()
{
	// The pixel shader outputs premultiplied alpha, hence ONE / INV_SRC_ALPHA.
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(device->CreateBlendState(&blendDesc, &blendState)))
		throw std::runtime_error("Failed to create the Gaussian splat blend state");

	// Splats are semi transparent, so they test against the scene depth but must
	// not write to it - otherwise the first splat drawn would hide the ones behind it.
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;

	if (FAILED(device->CreateDepthStencilState(&depthDesc, &depthReadOnlyState)))
		throw std::runtime_error("Failed to create the Gaussian splat depth state");

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthClipEnable = TRUE;
	device->CreateRasterizerState(&rasterizerDesc, &splatRS);
}

void GaussianSplatShader::loadVertexShader(const std::wstring& vertexShaderPath)
{
	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompileFromFile(
		vertexShaderPath.c_str(), nullptr, nullptr,
		"VSMain", "vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&vsBlob, &errorBlob);

	if (FAILED(hr))
		throw std::runtime_error("Gaussian splat vertex shader compilation failed");

	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "CORNER",     0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "SPLATINDEX", 0, DXGI_FORMAT_R32_UINT,     1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
}

void GaussianSplatShader::DrawSplats(const MatrixBuffer& matrices, const SplatFrameBuffer& frame, UINT instanceCount)
{
	if (instanceCount == 0 || !splatSRV || !orderBuffer || !vertexBuffer || !indexBuffer)
		return;

	UINT strides[2] = { sizeof(SplatCorner), sizeof(std::uint32_t) };
	UINT offsets[2] = { 0, 0 };
	ID3D11Buffer* vertexBuffers[2] = { vertexBuffer.Get(), orderBuffer.Get() };

	context->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &matrices, 0, 0);

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (SUCCEEDED(context->Map(frameBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		memcpy(mapped.pData, &frame, sizeof(SplatFrameBuffer));
		context->Unmap(frameBuffer.Get(), 0);
	}

	context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
	context->VSSetConstantBuffers(1, 1, frameBuffer.GetAddressOf());
	context->VSSetShaderResources(0, 1, splatSRV.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, frameBuffer.GetAddressOf());

	context->RSSetState(splatRS.Get());
	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShader.Get(), nullptr, 0);

	const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);
	context->OMSetDepthStencilState(depthReadOnlyState.Get(), 0);

	context->DrawIndexedInstanced(6, instanceCount, 0, 0, 0);
}

void GaussianSplatShader::RestoreDefaultStates()
{
	context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(nullptr, 0);

	ID3D11ShaderResourceView* noResource = nullptr;
	context->VSSetShaderResources(0, 1, &noResource);
}
