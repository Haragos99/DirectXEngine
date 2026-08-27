#include "pointcloudmodel.h"
#include "Ply/pointcloudplyloader.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

using namespace DirectX;

namespace
{
	// Half a spacing would leave holes, so overlap the neighbours slightly.
	constexpr float kRadiusFactor = 0.6f;
	constexpr float kMinRadius = 1e-4f;
}

PointCloudModel::PointCloudModel(
	const std::string& path,
	const std::wstring& vertexShaderPath,
	const std::wstring& pixelShaderPath,
	Microsoft::WRL::ComPtr<ID3D11Device> _device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
	: SplatCloudObject(_device, _context)
{
	name = std::filesystem::path(path).stem().string();

	PointCloud cloud;
	std::string error;
	if (!Ply::PointCloudPlyLoader().Load(path, cloud, &error))
	{
		std::cerr << "Failed to load PLY point cloud: " << error << '\n';
		return;
	}

	if (!Upload(vertexShaderPath, pixelShaderPath, BuildInstances(cloud), cloud.boundsMin, cloud.boundsMax))
	{
		std::cerr << "Failed to upload the point cloud of " << path << " to the GPU\n";
		return;
	}

	std::cout << "Loaded " << GetPointCount() << " points from " << path << '\n';
}

std::vector<SplatInstance> PointCloudModel::BuildInstances(const PointCloud& cloud)
{
	const size_t total = cloud.points.size();
	if (total == 0)
		return {};

	// Uniform stride keeps the cloud evenly covered when it has to be reduced.
	const size_t stride = SubsampleStride(total);

	// Subsampling spreads the remaining points further apart, so the radius has
	// to grow with it to keep the surface closed. Parenthesised std::max dodges
	// the windows.h macro.
	const float spacing = cloud.AverageSpacing() * std::cbrt(static_cast<float>(stride));
	const float radius = (std::max)(spacing * kRadiusFactor, kMinRadius);

	std::vector<SplatInstance> instances;
	instances.reserve((total + stride - 1) / stride);

	for (size_t i = 0; i < total; i += stride)
	{
		const PointCloudPoint& point = cloud.points[i];
		if (point.color.w <= 0.0f)
			continue;

		SplatInstance instance = {};
		instance.center = point.position;
		instance.scale = XMFLOAT3(radius, radius, radius);
		instance.rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // isotropic, so orientation is irrelevant
		instance.color = point.color;
		instances.push_back(instance);
	}

	return instances;
}
