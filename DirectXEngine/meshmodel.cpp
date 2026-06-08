#include "meshmodel.h"

MeshModel::MeshModel(std::string path, std::wstring VSPath, std::wstring PSPath,Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex) : Object3D(_device, _contex)
{
	mesh = Mesh();
	mesh.loadMesh(path);
	createTexturedVertex();
	createIndeces();
	texture->CreateSolidColorTexture({ 1, 0, 0, 1 });
	indices = mesh.indices;
	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	shader->createConstantBuffer();
	shader->creaetLightBuffer();
	shader->createRasterize();
	shader->LoadShaders(VSPath, PSPath);
	wireframeEnabled = false;
	Scale(0.5f, 0.5f, 0.5f);
}



void MeshModel::createIndeces()
{
	indices = mesh.indices;
}

void MeshModel::createTexturedVertex()
{

	vertices = mesh.vertices;
}



void MeshModel::Update(float time)
{
	//Rotate(0.0f, time * 0.5f, 0.0f);
}