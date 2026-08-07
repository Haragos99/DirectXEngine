#include "plane.h"

Plane::Plane(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex) : Object3D(_device, _contex)
{
	normalMap = std::make_unique<Texture>(device, context);
	createTexturedVertex();
	createIndeces();
	texture->LoadTextureFromFile(L"..\\Resources\\B_S.png");
	normalMap->LoadTextureFromFile(L"..\\Resources\\B_N.png");
	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	shader->createConstantBuffer();
	shader->creaetLightBuffer();
	shader->createRasterize();
	shader->LoadShaders(L"shaders\\PlaneVertexShader.hlsl", L"shaders\\PlanePixelShader.hlsl");
	wireframeEnabled = false;
	SetPosition(0.0f, -1.0f, 0.0f);
	Scale(5.0f, 1.0f, 5.0f);
	createWorldBoundingBox();
	name = "Plane";
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




void Plane::Update(float time)
{
	//Rotate(0.0f, time * 0.5f, 0.0f);
}