#include "object3d.h"
#include <stdexcept>
#include <d3dcompiler.h>

Object3D::Object3D(Microsoft::WRL::ComPtr<ID3D11Device> gfx)
	: world(DirectX::XMMatrixIdentity())
{
	// Define a simple triangle

	Vertex vertices[] = {
		{ { 0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;
	HRESULT hr = gfx->CreateBuffer(&bd, &initData, &vertexBuffer);
	if (FAILED(hr))
		throw std::runtime_error("Failed to create vertex buffer");

	// Load Shaders
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob; // This will hold compilation errors
	hr = D3DCompileFromFile(
		L"VertexShader.hlsl",  // file path
		nullptr, nullptr,
		"VSMain", "vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0,
		&vsBlob, &errorBlob);

	if (FAILED(hr))
	{

		throw std::runtime_error("Vertex Shader compilation failed");
	}
	 hr = hr = D3DCompileFromFile(
		 L"PixelShader.hlsl", nullptr, nullptr,
		 "PSMain", "ps_5_0",
		 D3DCOMPILE_ENABLE_STRICTNESS, 0,
		 &psBlob, &errorBlob
	 );

	 if (FAILED(hr))
	 {

		 throw std::runtime_error("Vertex Shader compilation failed");
	 }
	gfx->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
	gfx->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);


	// Input Layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,   D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	gfx->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

	// Constant Buffer
	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(MatrixBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	gfx->CreateBuffer(&cbd, nullptr, &constantBuffer);
}


void Object3D::Update(float time)
{

}






void Object3D::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> gfx, Camera camera)
{
	DirectX::XMMATRIX projection = camera.GetProjectionMatrix();
	DirectX::XMMATRIX view = camera.GetViewMatrix();
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	gfx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	gfx->IASetInputLayout(inputLayout.Get());
	gfx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfx->VSSetShader(vertexShader.Get(), nullptr, 0);
	gfx->PSSetShader(pixelShader.Get(), nullptr, 0);
	// Update constant buffer
	MatrixBuffer mb;
	mb.world = DirectX::XMMatrixTranspose(world);
	mb.view = DirectX::XMMatrixTranspose(view);
	mb.projection = DirectX::XMMatrixTranspose(projection);
	gfx->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &mb, 0, 0);
	gfx->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
	gfx->Draw(3, 0);
}