#include "isosurfacemodel.h"

IsoSurfaceModel::IsoSurfaceModel(MeshData meshData,
                                 std::string modelName,
                                 const std::wstring& VSPath,
                                 const std::wstring& PSPath,
                                 Microsoft::WRL::ComPtr<ID3D11Device> _device,
                                 Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
	: Object3D(_device, _context), data(std::move(meshData))
{
	createTexturedVertex();
	createIndeces();

	texture->CreateSolidColorTexture({ 0.35f, 0.75f, 0.95f, 1.0f });
	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	shader->createConstantBuffer();
	shader->creaetLightBuffer();
	shader->createRasterize();
	shader->LoadShaders(VSPath, PSPath);

	wireframeEnabled = false;
	createWorldBoundingBox();
	name = std::move(modelName);
}

void IsoSurfaceModel::createTexturedVertex()
{
	vertices = data.vertices;
}

void IsoSurfaceModel::createIndeces()
{
	indices = data.indices;
}

void IsoSurfaceModel::Update(float /*time*/)
{
}
