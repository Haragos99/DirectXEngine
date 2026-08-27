#pragma once
#include "splatcloudobject.h"
#include "Ply/pointcloud.h"

#include <string>

// Scene object for a regular (non Gaussian) PLY point cloud.
//
// It renders through the same splat pipeline as GaussianSplatting, but a plain
// point carries no covariance, so every point becomes an isotropic splat whose
// radius is derived from the average spacing of the cloud.
class PointCloudModel : public SplatCloudObject
{
public:
	PointCloudModel(
		const std::string& path,
		const std::wstring& vertexShaderPath,
		const std::wstring& pixelShaderPath,
		Microsoft::WRL::ComPtr<ID3D11Device> _device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	size_t GetPointCount() const { return GetSplatCount(); }

private:
	static std::vector<SplatInstance> BuildInstances(const PointCloud& cloud);
};
