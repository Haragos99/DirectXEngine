#pragma once
#include "iobjectimporter.h"

// Any format the ModelLoaderRegistry can parse into triangles (OBJ, polygonal
// PLY, and whatever IModelLoader is registered next) becomes a MeshModel.
class MeshObjectImporter : public IObjectImporter
{
public:
	const char* GetFormatName() const override { return "Polygon mesh"; }
	bool CanImport(const std::string& path) const override;
	std::shared_ptr<Object3D> Import(
		const std::string& path,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const override;
};
