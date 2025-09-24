#include "cube.h"

Cube::Cube(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex) : Object3D(_device, _contex)
{
	createTexturedVertex();
	createIndeces();
	texture->LoadTextureFromFile(L"..\\Resources\\wood.png");
	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	shader->createConstantBuffer();
	shader->creaetLightBuffer();
	shader->createRasterize();
	shader->LoadShaders(L"shaders\\VertexShader.hlsl", L"shaders\\PixelShader.hlsl");
	wireframeEnabled = false;
}

void Cube::createIndeces()
{
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
}


void Cube::createTexturedVertex()
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

void Cube::Draw(Camera camera)
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

void Cube::Update(float time)
{
	Rotate(0.0f, time * 0.5f, 0.0f);
}