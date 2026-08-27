#pragma once
#include "iobjectimporter.h"

// A .ply with vertices but no faces and no splat attributes becomes a
// PointCloudModel.
class PointCloudImporter : public IObjectImporter
{
public:
	const char* GetFormatName() const override { return "Point cloud"; }
	bool CanImport(const std::string& path) const override;
	std::shared_ptr<Object3D> Import(
		const std::string& path,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) const override;
};
