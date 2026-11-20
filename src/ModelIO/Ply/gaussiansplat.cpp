#include "gaussiansplat.h"

#include <algorithm>
#include <limits>

using namespace DirectX;

XMFLOAT3 GaussianSplatCloud::Center() const
{
	return XMFLOAT3(
		(boundsMin.x + boundsMax.x) * 0.5f,
		(boundsMin.y + boundsMax.y) * 0.5f,
		(boundsMin.z + boundsMax.z) * 0.5f);
}

float GaussianSplatCloud::Extent() const
{
	return std::max({ boundsMax.x - boundsMin.x, boundsMax.y - boundsMin.y, boundsMax.z - boundsMin.z });
}

void GaussianSplatCloud::ComputeBounds()
{
	if (splats.empty())
	{
		boundsMin = XMFLOAT3(0.0f, 0.0f, 0.0f);
		boundsMax = XMFLOAT3(0.0f, 0.0f, 0.0f);
		return;
	}

	constexpr float largest = std::numeric_limits<float>::max();
	boundsMin = XMFLOAT3(largest, largest, largest);
	boundsMax = XMFLOAT3(-largest, -largest, -largest);

	for (const GaussianSplat& splat : splats)
	{
		boundsMin.x = std::min(boundsMin.x, splat.center.x);
		boundsMin.y = std::min(boundsMin.y, splat.center.y);
		boundsMin.z = std::min(boundsMin.z, splat.center.z);
		boundsMax.x = std::max(boundsMax.x, splat.center.x);
		boundsMax.y = std::max(boundsMax.y, splat.center.y);
		boundsMax.z = std::max(boundsMax.z, splat.center.z);
	}
}
