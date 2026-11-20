#pragma once
#include "object3d.h"

#include <d3d11.h>
#include <memory>
#include <string>
#include <wrl.h>

// Turns a model file into the scene object that knows how to render it.
//
// A file extension alone is not enough to pick a renderer: a .ply can be a
// mesh, a Gaussian splat capture or a loose point cloud. Each importer
// therefore inspects the file itself. Supporting a new format means adding an
// implementation and registering it with ModelImportService - no existing code
// has to change.
class IObjectImporter
{
public:
	virtual ~IObjectImporter() = default;

	// Human readable name of what this importer produces, used in log messages.
	virtual const char* GetFormatName() const = 0;

	// True when this importer recognises the file and wants to handle it.
	virtual bool CanImport(const std::string& path) const = 0;

	// Builds the scene object, or returns nullptr when the file fails to load.
	virtual std::shared_ptr<Object3D> Import(
		const std::string& path,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const = 0;
};
