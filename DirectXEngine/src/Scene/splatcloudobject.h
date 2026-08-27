#pragma once
#include "object3d.h"
#include "gaussiansplatshader.h"
#include "splatdepthsorter.h"

#include <cstdint>
#include <vector>

// Shared behaviour of every scene object drawn as a cloud of screen aligned
// splats: GPU buffer setup, per-frame back-to-front depth sorting and the
// instanced draw call. Subclasses only decide how a file becomes a list of
// SplatInstances, which is what separates a Gaussian capture from a plain
// point cloud.
class SplatCloudObject : public Object3D
{
public:
	void Update(float time) override;
	void Draw(Camera camera, RenderMode mode) override;

	bool IsLoaded() const { return splatCount > 0; }
	size_t GetSplatCount() const { return splatCount; }
	void SetSplatScale(float scale) { splatScale = scale; }

	// Clouds larger than this are uniformly subsampled so the GPU buffers stay
	// within a sane amount of video memory.
	static constexpr size_t MaxSplats = 1500000;

protected:
	SplatCloudObject(
		Microsoft::WRL::ComPtr<ID3D11Device> _device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	// Builds the splat pipeline and uploads the cloud. Returns false when the
	// cloud is empty or the GPU refuses the buffers.
	bool Upload(
		const std::wstring& vertexShaderPath,
		const std::wstring& pixelShaderPath,
		const std::vector<SplatInstance>& instances,
		const DirectX::XMFLOAT3& boundsMin,
		const DirectX::XMFLOAT3& boundsMax);

	// Step size that keeps `total` points under MaxSplats.
	static size_t SubsampleStride(size_t total);

	// Splats are instanced quads, so there is no shared vertex or index array.
	void createTexturedVertex() override {}
	void createIndeces() override {}

private:
	void CreateBounds(const DirectX::XMFLOAT3& boundsMin, const DirectX::XMFLOAT3& boundsMax);
	SplatFrameBuffer BuildFrameConstants(const Camera& camera) const;

	GaussianSplatShader* splatShader = nullptr; // owned by Object3D::shader
	// Splat centres kept on the CPU purely to feed the per-frame depth sort.
	std::vector<DirectX::XMFLOAT3> sortCenters;
	std::vector<std::uint32_t> drawOrder;
	SplatDepthSorter sorter;
	float sceneExtent = 1.0f;
	size_t splatCount = 0;
	float splatScale = 1.0f;
};
