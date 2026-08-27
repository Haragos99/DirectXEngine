#include "modelimportservice.h"
#include "Importers/gaussiansplatimporter.h"
#include "Importers/meshobjectimporter.h"
#include "Importers/pointcloudimporter.h"

#include <filesystem>
#include <iostream>

ModelImportService& ModelImportService::Instance()
{
	static ModelImportService service = []
	{
		ModelImportService created;
		// Splat and point cloud checks read the PLY header, so they run before
		// the mesh importer, which accepts any registered mesh extension.
		created.Register(std::make_shared<GaussianSplatImporter>());
		created.Register(std::make_shared<PointCloudImporter>());
		created.Register(std::make_shared<MeshObjectImporter>());
		return created;
	}();
	return service;
}

void ModelImportService::Register(std::shared_ptr<IObjectImporter> importer)
{
	if (importer)
		importers.push_back(std::move(importer));
}

std::shared_ptr<Object3D> ModelImportService::Import(
	const std::string& path,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const
{
	if (path.empty())
		return nullptr;

	for (const std::shared_ptr<IObjectImporter>& importer : importers)
	{
		if (!importer->CanImport(path))
			continue;

		if (auto imported = importer->Import(path, device, context))
			return imported;

		std::cerr << importer->GetFormatName() << " importer failed on: " << path << '\n';
		return nullptr;
	}

	std::cerr << "No importer can handle: " << path << '\n';
	return nullptr;
}

std::shared_ptr<Object3D> ModelImportService::Import(
	const std::wstring& path,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const
{
	return Import(std::filesystem::path(path).string(), device, context);
}
