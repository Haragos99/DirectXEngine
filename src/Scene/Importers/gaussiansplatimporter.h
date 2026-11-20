#pragma once
#include "iobjectimporter.h"

// A .ply carrying Gaussian splat attributes becomes a GaussianSplatting object.
class GaussianSplatImporter : public IObjectImporter
{
public:
	const char* GetFormatName() const override { return "Gaussian splat cloud"; }
	bool CanImport(const std::string& path) const override;
	std::shared_ptr<Object3D> Import(
		const std::string& path,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const override;
};
