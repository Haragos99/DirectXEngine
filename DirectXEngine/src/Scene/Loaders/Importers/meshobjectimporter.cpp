#include "meshobjectimporter.h"
#include "meshmodel.h"
#include "modelloaderregistry.h"

bool MeshObjectImporter::CanImport(const std::string& path) const
{
	return ModelLoaderRegistry::Instance().FindLoaderFor(path) != nullptr;
}

std::shared_ptr<Object3D> MeshObjectImporter::Import(
	const std::string& path,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const
{
	auto model = std::make_shared<MeshModel>(
		path, L"shaders\\VertexShader.hlsl", L"shaders\\MeshPixelShader.hlsl", device, context);
	model->SetPosition(0.0f, -1.0f, 0.0f);
	return model;
}
