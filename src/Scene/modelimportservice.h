#pragma once
#include "Importers/iobjectimporter.h"

#include <memory>
#include <string>
#include <vector>

// Single entry point for getting a model file into the scene.
//
// It asks every registered IObjectImporter, in order, whether it recognises the
// file and returns the scene object built by the first one that does. Callers
// only supply a path: which loader runs, and which Object3D comes back, is
// decided here.
class ModelImportService
{
public:
	// Process-wide service pre-populated with the built-in importers.
	static ModelImportService& Instance();

	ModelImportService() = default;

	// Importers are tried in registration order, so register the ones with the
	// most specific file checks first.
	void Register(std::shared_ptr<IObjectImporter> importer);

	// Scene object for `path`, or nullptr when no importer handles the file or
	// the file fails to load.
	std::shared_ptr<Object3D> Import(
		const std::string& path,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const;

	std::shared_ptr<Object3D> Import(
		const std::wstring& path,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const;

private:
	std::vector<std::shared_ptr<IObjectImporter>> importers;
};
