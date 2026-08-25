#pragma once
#include "imodelloader.h"
#include <memory>
#include <string>
#include <vector>

// Resolves a model file to the loader that understands its format.
// Registering a new IModelLoader is the only step needed to support an
// additional file format.
class ModelLoaderRegistry
{
public:
	// Process-wide registry pre-populated with the built-in loaders.
	static ModelLoaderRegistry& Instance();

	ModelLoaderRegistry() = default;

	void Register(std::shared_ptr<IModelLoader> loader);

	// Loader matching the file extension of `path`, or nullptr when unsupported.
	std::shared_ptr<IModelLoader> FindLoaderFor(const std::string& path) const;

	// Convenience: resolve the loader and parse the file in one step.
	bool Load(const std::string& path, MeshData& out) const;

	// Win32 OPENFILENAME filter string (double-null terminated) covering every
	// registered format, so the file dialog stays in sync with the loaders.
	std::wstring BuildFileDialogFilter() const;

	// Lowercase extension of `path` without the dot ("" when there is none).
	static std::string ExtensionOf(const std::string& path);

private:
	std::vector<std::shared_ptr<IModelLoader>> loaders;
};
