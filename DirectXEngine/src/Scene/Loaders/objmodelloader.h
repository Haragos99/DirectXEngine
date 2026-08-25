#pragma once
#include "imodelloader.h"

// Wavefront OBJ implementation of IModelLoader.
class ObjModelLoader : public IModelLoader
{
public:
	const char* GetFormatName() const override { return "Wavefront OBJ"; }
	std::vector<std::string> GetSupportedExtensions() const override { return { "obj" }; }
	bool Load(const std::string& path, MeshData& out) const override;
};
