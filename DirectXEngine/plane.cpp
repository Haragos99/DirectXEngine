#include "plane.h"

Plane::Plane(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex) : Object3D(_device, _contex)
{
	createTexturedVertex();
	createIndeces();
	texture->LoadTextureFromFile(L"..\\Resources\\moss.png");
	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	shader->createConstantBuffer();
	shader->creaetLightBuffer();
	shader->createRasterize();
	shader->LoadShaders(L"shaders\\VertexShader.hlsl", L"shaders\\PixelShader.hlsl");
	wireframeEnabled = false;
	SetPosition(0.0f, -1.0f, 0.0f);
	Scale(5.0f, 1.0f, 5.0f);
}


void Plane::createIndeces()
{
	indices = {
		0, 1, 2, 0, 2, 3
	};
}

void Plane::createTexturedVertex()
{
	vertices.clear();
	vertices = {
		// Plane in the XZ plane (Y up)
		  { {-5,0,-5}, {0,1,0}, {0,5} }, { {-5,0,5}, {0,1,0}, {0,0} },
		  { {5,0,5},  {0,1,0}, {5,0} }, { {5,0,-5}, {0,1,0}, {5,5} },
	};
}



void Plane::Draw(Camera camera)
{
	DirectX::XMMATRIX projection = camera.GetProjectionMatrix();
	DirectX::XMMATRIX view = camera.GetViewMatrix();
	texture->Use();
	MatrixBuffer mb;
	mb.world = DirectX::XMMatrixTranspose(world);
	mb.view = DirectX::XMMatrixTranspose(view);
	mb.projection = DirectX::XMMatrixTranspose(projection);
	LightBuffer lightData;
	lightData.lightDirection = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f); // example
	lightData.padding = 0.0f;
	lightData.lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // white light
	shader->renderDraw(mb, lightData);
	context->DrawIndexed(indices.size(), 0, 0);
	// Wireframe overlay
	//context->RSSetState(wireframeRS.Get());
	//context->PSSetShader(blackPixelShader.Get(), nullptr, 0);
	//context->DrawIndexed(indices.size(), 0, 0);
}

void Plane::Update(float time)
{
	Rotate(0.0f, time * 0.5f, 0.0f);
}