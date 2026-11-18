#include "meshmodel.h"
#include <iostream>
#include <filesystem>

namespace
{
	// File name without directory or extension, used as the scene object name.
	std::string StemOf(const std::string& path)
	{
		return std::filesystem::path(path).stem().string();
	}
}

MeshModel::MeshModel(std::string path, std::wstring VSPath, std::wstring PSPath,Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex) : Object3D(_device, _contex)
{
	if (!mesh.loadMesh(path))
	{
		std::cerr << "No model loader could handle: " << path << '\n';
	}
	createTexturedVertex();
	createIndeces();
	texture->CreateSolidColorTexture({ 1, 0, 0, 1 });
	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	shader->createConstantBuffer();
	shader->creaetLightBuffer();
	shader->createRasterize();
	shader->LoadShaders(VSPath, PSPath);
	wireframeEnabled = false;
	Scale(0.5f, 0.5f, 0.5f);
	createWorldBoundingBox();
	name = StemOf(path);
}

void MeshModel::createIndeces()
{
	indices = mesh.GetIndices();
}

void MeshModel::createTexturedVertex()
{
	vertices = mesh.GetVertices();
}

void MeshModel::Update(float time)
{
	//Rotate(0.0f, time * 0.5f, 0.0f);
}