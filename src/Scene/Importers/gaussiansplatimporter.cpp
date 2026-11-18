#include "gaussiansplatimporter.h"
#include "gaussiansplatting.h"
#include "Ply/gaussiansplatplyloader.h"

bool GaussianSplatImporter::CanImport(const std::string& path) const
{
	return Ply::GaussianSplatPlyLoader::IsSplatFile(path);
}

std::shared_ptr<Object3D> GaussianSplatImporter::Import(
	const std::string& path,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const
{
	auto cloud = std::make_shared<GaussianSplatting>(
		path, L"shaders\\GaussianSplatVS.hlsl", L"shaders\\GaussianSplatPS.hlsl", device, context);
	return cloud->IsLoaded() ? cloud : nullptr;
}
