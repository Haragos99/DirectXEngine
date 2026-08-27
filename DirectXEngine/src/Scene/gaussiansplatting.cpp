#include "gaussiansplatting.h"
#include "Ply/gaussiansplatplyloader.h"

#include <filesystem>
#include <iostream>

using namespace DirectX;

GaussianSplatting::GaussianSplatting(
	const std::string& path,
	const std::wstring& vertexShaderPath,
	const std::wstring& pixelShaderPath,
	Microsoft::WRL::ComPtr<ID3D11Device> _device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
	: SplatCloudObject(_device, _context)
{
	name = std::filesystem::path(path).stem().string();

	GaussianSplatCloud cloud;
	std::string error;
	if (!Ply::GaussianSplatPlyLoader().Load(path, cloud, &error))
	{
		std::cerr << "Failed to load Gaussian splat PLY: " << error << '\n';
		return;
	}

	if (!Upload(vertexShaderPath, pixelShaderPath, BuildInstances(cloud), cloud.boundsMin, cloud.boundsMax))
	{
		std::cerr << "Failed to upload the Gaussian splats of " << path << " to the GPU\n";
		return;
	}

	std::cout << "Loaded " << GetSplatCount() << " Gaussian splats from " << path << '\n';

	// Captures use a Y-down convention, so flip them upright.
	Rotate(XM_PI, 0.0f, 0.0f);
}

std::vector<SplatInstance> GaussianSplatting::BuildInstances(const GaussianSplatCloud& cloud)
{
	const size_t total = cloud.splats.size();
	if (total == 0)
		return {};

	// Uniform stride keeps the cloud evenly covered when it has to be reduced.
	const size_t stride = SubsampleStride(total);

	std::vector<SplatInstance> instances;
	instances.reserve((total + stride - 1) / stride);

	for (size_t i = 0; i < total; i += stride)
	{
		const GaussianSplat& splat = cloud.splats[i];
		if (splat.color.w <= 0.0f)
			continue;

		SplatInstance instance = {};
		instance.center = splat.center;
		instance.scale = splat.scale;
		instance.rotation = splat.rotation;
		instance.color = splat.color;
		instances.push_back(instance);
	}

	return instances;
}
