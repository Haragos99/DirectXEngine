#pragma once
#include "splatcloudobject.h"
#include "Ply/gaussiansplat.h"

#include <string>

// Scene object for a 3D Gaussian Splatting capture loaded from a .ply file.
// It plays the same role for splat clouds that MeshModel plays for polygonal
// models: load the file, upload the GPU buffers and draw itself.
class GaussianSplatting : public SplatCloudObject
{
public:
	GaussianSplatting(
		const std::string& path,
		const std::wstring& vertexShaderPath,
		const std::wstring& pixelShaderPath,
		Microsoft::WRL::ComPtr<ID3D11Device> _device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

private:
	// Each Gaussian already carries its own anisotropic scale and rotation.
	static std::vector<SplatInstance> BuildInstances(const GaussianSplatCloud& cloud);
};
