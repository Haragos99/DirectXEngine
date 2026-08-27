#include "splatcloudobject.h"

#include <algorithm>

using namespace DirectX;

SplatCloudObject::SplatCloudObject(
	Microsoft::WRL::ComPtr<ID3D11Device> _device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
	: Object3D(_device, _context)
{
	wireframeEnabled = false;
}

size_t SplatCloudObject::SubsampleStride(size_t total)
{
	return total <= MaxSplats ? 1 : (total + MaxSplats - 1) / MaxSplats;
}

bool SplatCloudObject::Upload(
	const std::wstring& vertexShaderPath,
	const std::wstring& pixelShaderPath,
	const std::vector<SplatInstance>& instances,
	const XMFLOAT3& boundsMin,
	const XMFLOAT3& boundsMax)
{
	if (instances.empty())
		return false;

	// Replace the generic shader created by Object3D with the splat pipeline,
	// keeping a typed view on it for the specialised draw call.
	auto pipeline = std::make_unique<GaussianSplatShader>(device, context);
	splatShader = pipeline.get();
	shader = std::move(pipeline);

	splatShader->CreateQuadGeometry();
	splatShader->createConstantBuffer();
	splatShader->CreateFrameBuffer();
	splatShader->CreateBlendState();
	splatShader->LoadShaders(vertexShaderPath, pixelShaderPath);

	if (!splatShader->CreateSplatBuffer(instances) || !splatShader->CreateOrderBuffer(instances.size()))
		return false;

	sortCenters.clear();
	sortCenters.reserve(instances.size());
	for (const SplatInstance& instance : instances)
		sortCenters.push_back(instance.center);

	splatCount = instances.size();
	CreateBounds(boundsMin, boundsMax);
	return true;
}

void SplatCloudObject::CreateBounds(const XMFLOAT3& boundsMin, const XMFLOAT3& boundsMax)
{
	// Parenthesised to dodge the windows.h max macro.
	sceneExtent = (std::max)({ boundsMax.x - boundsMin.x, boundsMax.y - boundsMin.y, boundsMax.z - boundsMin.z, 1e-3f });

	// Built from the cloud bounds instead of Object3D::createWorldBoundingBox,
	// which would need a CPU copy of every splat centre.
	BoundingBox aabb;
	BoundingBox::CreateFromPoints(aabb, XMLoadFloat3(&boundsMin), XMLoadFloat3(&boundsMax));
	BoundingOrientedBox::CreateFromBoundingBox(localBox, aabb);
	updateWorldBoundingBox();
}

SplatFrameBuffer SplatCloudObject::BuildFrameConstants(const Camera& camera) const
{
	UINT viewportCount = 1;
	D3D11_VIEWPORT viewport = {};
	context->RSGetViewports(&viewportCount, &viewport);

	const float width = viewport.Width > 0.0f ? viewport.Width : 1.0f;
	const float height = viewport.Height > 0.0f ? viewport.Height : 1.0f;

	XMFLOAT4X4 projection;
	XMStoreFloat4x4(&projection, camera.GetProjectionMatrix());

	SplatFrameBuffer frame = {};
	frame.viewportSize = XMFLOAT2(width, height);
	// Perspective projection scale converted to pixels.
	frame.focal = XMFLOAT2(width * 0.5f * projection._11, height * 0.5f * projection._22);
	frame.splatScale = splatScale;
	frame.alphaCutoff = 1.0f / 255.0f;
	frame.padding = XMFLOAT2(0.0f, 0.0f);
	return frame;
}

void SplatCloudObject::Draw(Camera camera, RenderMode mode)
{
	// Splats have no edges, so every render mode draws the same way.
	(void)mode;

	if (splatShader == nullptr || splatCount == 0)
		return;

	const XMMATRIX viewMatrix = camera.GetViewMatrix();

	// Alpha blending is order dependent, so the splats have to reach the
	// rasteriser back to front for this view.
	if (sorter.Update(sortCenters, XMMatrixMultiply(world, viewMatrix), sceneExtent, drawOrder))
		splatShader->UpdateOrder(drawOrder);

	MatrixBuffer matrices;
	matrices.world = XMMatrixTranspose(world);
	matrices.view = XMMatrixTranspose(viewMatrix);
	matrices.projection = XMMatrixTranspose(camera.GetProjectionMatrix());

	splatShader->DrawSplats(matrices, BuildFrameConstants(camera), static_cast<UINT>(splatCount));
	splatShader->RestoreDefaultStates();
}

void SplatCloudObject::Update(float time)
{
	(void)time;
}
