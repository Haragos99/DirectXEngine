#include "pointcloudimporter.h"
#include "pointcloudmodel.h"
#include "Ply/pointcloudplyloader.h"

bool PointCloudImporter::CanImport(const std::string& path) const
{
	return Ply::PointCloudPlyLoader::IsPointCloudFile(path);
}

std::shared_ptr<Object3D> PointCloudImporter::Import(
	const std::string& path,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const
{
	auto cloud = std::make_shared<PointCloudModel>(
		path, L"shaders\\GaussianSplatVS.hlsl", L"shaders\\GaussianSplatPS.hlsl", device, context);
	return cloud->IsLoaded() ? cloud : nullptr;
}
