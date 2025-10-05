#include "meshmodel.h"

MeshModel::MeshModel(std::string path,Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex) : Object3D(_device, _contex)
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
	shader->LoadShaders(L"shaders\\VertexShader.hlsl", L"shaders\\PixelShader.hlsl");
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


void MeshModel::Draw(Camera camera)
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
	if (wireframeEnabled)
	{
		context->RSSetState(shader->GetwireframeRS().Get());
		context->PSSetShader(shader->GetBlackPixelShader().Get(), nullptr, 0);
		context->DrawIndexed(indices.size(), 0, 0);
	}
}

void MeshModel::Update(float time)
{
	Rotate(0.0f, time * 0.5f, 0.0f);
}