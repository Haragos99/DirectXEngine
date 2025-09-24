#include "object3d.h"
#include <stdexcept>
#include <d3dcompiler.h>
using namespace DirectX;

Object3D::Object3D(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contexx) : world(DirectX::XMMatrixIdentity()), device(_device), context(_contexx)
{
	createTexturedCube();
	texture = std::make_unique<Texture>(device, context);
	texture->LoadTextureFromFile(L"..\\Resources\\wood.png");
	// 12 triangles  36 indices
	indices = {
		// Front (-Z)
		0, 1, 2, 0, 2, 3,
		// Back (+Z)
		4, 5, 6, 4, 6, 7,
		// Left (-X)
		8, 9, 10, 8, 10, 11,
		// Right (+X)
		12, 13, 14, 12, 14, 15,
		// Top (+Y)
		16, 17, 18, 16, 18, 19,
		// Bottom (-Y)
		20, 21, 22, 20, 22, 23
	};
	createVertexBuffer();
	createInexxBuffer();
	loadShaders();
	createConstantBuffer();
	creaetLightBuffer();
	wireframeEnabled = false;
}


void Object3D::createConstantBuffer()
{
	// Constant Buffer
	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(MatrixBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	device->CreateBuffer(&cbd, nullptr, &constantBuffer);
}

void Object3D::loadShaders()
{
	std::wstring shaderPathV = L"shaders\\VertexShader.hlsl";
	std::wstring shaderPathP = L"shaders\\PixelShader.hlsl";
	std::wstring shaderPathB = L"shaders\\BlackPixelShader.hlsl";

	// Load Shaders
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob; // This will hold compilation errors
	HRESULT hr = D3DCompileFromFile(
		shaderPathV.c_str(),  // file path
		nullptr, nullptr,
		"VSMain", "vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&vsBlob, &errorBlob
	);

	if (FAILED(hr))
	{

		throw std::runtime_error("Vertex Shader compilation failed");
	}
	hr = D3DCompileFromFile(
		shaderPathP.c_str(), nullptr, nullptr,
		"PSMain", "ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&psBlob, &errorBlob
	);
	if (FAILED(hr))
	{

		throw std::runtime_error("Vertex Shader compilation failed");
	}

	// Black pixel shader (for wireframe)
	Microsoft::WRL::ComPtr<ID3DBlob> psBlobB;
	D3DCompileFromFile(shaderPathB.c_str(), nullptr, nullptr,
		"PSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&psBlobB, nullptr);

	device->CreatePixelShader(psBlobB->GetBufferPointer(), psBlobB->GetBufferSize(), nullptr, &blackPixelShader);
	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

	// Input Layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
}


void Object3D::createVertexBuffer()
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = (vertices.size() * sizeof(Vertex));
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices.data();
	HRESULT hr = device->CreateBuffer(&bd, &initData, &vertexBuffer);
	if (FAILED(hr))
		throw std::runtime_error("Failed to create vertex buffer");
}

void Object3D::createInexxBuffer()
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
		throw std::runtime_error("Failed to create index buffer");
}


void Object3D::creaetLightBuffer()
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
		throw std::runtime_error("Failed to create light buffer");
}

void Object3D::createRasterize()
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


void Object3D::createTexturedCube()
{
	vertices.clear();

	vertices = {
		// Front (-Z)
		  { {-1,-1,-1}, {0,0,-1}, {0,1} }, { {-1,1,-1}, {0,0,-1}, {0,0} },
		  { {1,1,-1},  {0,0,-1}, {1,0} }, { {1,-1,-1}, {0,0,-1}, {1,1} },
		  // Back (+Z)
		  { {-1,-1,1}, {0,0,1}, {1,1} }, { {1,-1,1}, {0,0,1}, {0,1} },
		  { {1,1,1},   {0,0,1}, {0,0} }, { {-1,1,1}, {0,0,1}, {1,0} },
		  // Left (-X)
		  { {-1,-1,1}, {-1,0,0}, {0,1} }, { {-1,1,1}, {-1,0,0}, {0,0} },
		  { {-1,1,-1}, {-1,0,0}, {1,0} }, { {-1,-1,-1}, {-1,0,0}, {1,1} },
		  // Right (+X)
		  { {1,-1,-1}, {1,0,0}, {0,1} }, { {1,1,-1}, {1,0,0}, {0,0} },
		  { {1,1,1},   {1,0,0}, {1,0} }, { {1,-1,1}, {1,0,0}, {1,1} },
		  // Top (+Y)
		  { {-1,1,-1}, {0,1,0}, {0,1} }, { {-1,1,1}, {0,1,0}, {0,0} },
		  { {1,1,1},   {0,1,0}, {1,0} }, { {1,1,-1}, {0,1,0}, {1,1} },
		  // Bottom (-Y)
		  { {-1,-1,1}, {0,-1,0}, {1,1} }, { {-1,-1,-1}, {0,-1,0}, {0,1} },
		  { {1,-1,-1}, {0,-1,0}, {0,0} }, { {1,-1,1}, {0,-1,0}, {1,0} },
	};
}



void Object3D::Update(float time)
{

}


void Object3D::Draw( Camera camera)
{
	DirectX::XMMATRIX projection = camera.GetProjectionMatrix();
	DirectX::XMMATRIX view = camera.GetViewMatrix();
	texture->Use();
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Constant buffer update
	MatrixBuffer mb;
	mb.world = DirectX::XMMatrixTranspose(world);
	mb.view = DirectX::XMMatrixTranspose(view);
	mb.projection = DirectX::XMMatrixTranspose(projection);
	context->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &mb, 0, 0);
	context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

	LightBuffer lightData;
	lightData.lightDirection = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f); // example
	lightData.padding = 0.0f;
	lightData.lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // white light


	// Map -> copy -> Unmap (correct for D3D11_USAGE_DYNAMIC)
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = context->Map(lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (SUCCEEDED(hr))
	{
		memcpy(mappedResource.pData, &lightData, sizeof(LightBuffer));
		context->Unmap(lightBuffer.Get(), 0);
	}
	else
	{
		// Optional: handle/log the error
	}
	context->PSSetConstantBuffers(1, 1, lightBuffer.GetAddressOf());

	// Solid
	context->RSSetState(solidRS.Get());
	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShader.Get(), nullptr, 0);
	context->DrawIndexed(indices.size(), 0, 0);

	// Wireframe overlay
	//context->RSSetState(wireframeRS.Get());
	//context->PSSetShader(blackPixelShader.Get(), nullptr, 0);

	//context->DrawIndexed(indices.size(), 0, 0);
}
