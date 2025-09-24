#include "shaders.h"

#include <d3dcompiler.h>

#include <stdexcept>


Shader::Shader(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext>  _context) : device(_device), context(_context)
{

}




void Shader::LoadShaders(const std::wstring& vertexShaderPath, const std::wstring& pixelShaderPath)
{
	// Load Shaders
	loadPixelShader(pixelShaderPath);
	loadVertexShader(vertexShaderPath);
	loadwireframePixelShader();
}


void Shader::loadVertexShader(const std::wstring& vertexShaderPath)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob; // This will hold compilation errors
	HRESULT hr = D3DCompileFromFile(
		vertexShaderPath.c_str(),  // file path
		nullptr, nullptr,
		"VSMain", "vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&vsBlob, &errorBlob
	);

	if (FAILED(hr))
	{
		throw std::runtime_error("Vertex Shader compilation failed");
	}
	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

	// Input Layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

}
void Shader::loadPixelShader(const std::wstring& pixelShaderPath)
{
	Microsoft::WRL::ComPtr<ID3DBlob>  psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob; // This will hold compilation errors
	HRESULT hr = D3DCompileFromFile(
		pixelShaderPath.c_str(), nullptr, nullptr,
		"PSMain", "ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&psBlob, &errorBlob
	);
	if (FAILED(hr))
	{
		throw std::runtime_error("Vertex Shader compilation failed");
	}
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
}
void Shader::loadwireframePixelShader()
{
	// Black pixel shader (for wireframe)
	Microsoft::WRL::ComPtr<ID3DBlob> psBlobB;
	D3DCompileFromFile(L"shaders\\BlackPixelShader.hlsl", nullptr, nullptr,
		"PSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&psBlobB, nullptr);

	device->CreatePixelShader(psBlobB->GetBufferPointer(), psBlobB->GetBufferSize(), nullptr, &blackPixelShader);
}


void Shader::createVertexBuffer(std::vector<Vertex> vertices)
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = (vertices.size() * sizeof(Vertex));
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices.data();
	HRESULT hr = device->CreateBuffer(&bd, &initData, &vertexBuffer);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create vertex buffer");
	}	
}

void Shader::createInexxBuffer(std::vector<UINT> indices)
{
	// Index Buffer
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = indices.size() * sizeof(UINT);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA iinitData = {};
	iinitData.pSysMem = indices.data();

	HRESULT hr = device->CreateBuffer(&ibd, &iinitData, &indexBuffer);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create index buffer");
	}	
}


void Shader::creaetLightBuffer()
{
	// Light Buffer
	D3D11_BUFFER_DESC lightBufferDesc = {};
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBuffer);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	HRESULT hr = device->CreateBuffer(&lightBufferDesc, nullptr, &lightBuffer);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create light buffer");
	}
}

void Shader::createRasterize()
{
	// Rasterizer states
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.DepthClipEnable = TRUE;
	device->CreateRasterizerState(&rsDesc, &solidRS);

	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	rsDesc.CullMode = D3D11_CULL_NONE; // Show all edges
	device->CreateRasterizerState(&rsDesc, &wireframeRS);
}



void Shader::createConstantBuffer()
{
	// Constant Buffer
	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(MatrixBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HRESULT hr = device->CreateBuffer(&cbd, nullptr, &constantBuffer);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create constant buffer");
	}
}


void Shader::renderDraw(MatrixBuffer& mb, LightBuffer& lightData)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	// Bind the vertex buffer to the pipeline's Input Assembler stage
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	// Bind the index buffer to the pipeline's Input Assembler stage
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	// Bind the input layout to the pipeline's Input Assembler stage
	context->IASetInputLayout(inputLayout.Get());
	// Bind the geometry topology to the pipeline's Input Assembler stage
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// Constant buffer update
	context->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &mb, 0, 0);
	context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());


	// Map  copy  Unmap (correct for D3D11_USAGE_DYNAMIC)
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = context->Map(lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (SUCCEEDED(hr))
	{
		memcpy(mappedResource.pData, &lightData, sizeof(LightBuffer));
		context->Unmap(lightBuffer.Get(), 0);
	}
	context->PSSetConstantBuffers(1, 1, lightBuffer.GetAddressOf());

	// Solid
	context->RSSetState(solidRS.Get());
	// Bind the vertex shader to the pipeline's Vertex Shader stage
	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	// Bind the pixel shader to the pipeline's Pixel Shader stage
	context->PSSetShader(pixelShader.Get(), nullptr, 0);
}