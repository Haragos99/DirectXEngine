#pragma once
#include "imodelloader.h"

// Polygonal PLY implementation of IModelLoader (Stanford triangle format).
// Gaussian splat PLY files are handled separately by Ply::GaussianSplatPlyLoader.
class PlyModelLoader : public IModelLoader
{
public:
	const char* GetFormatName() const override { return "Stanford PLY"; }
	std::vector<std::string> GetSupportedExtensions() const override { return { "ply" }; }
	bool Load(const std::string& path, MeshData& out) const override;
};
