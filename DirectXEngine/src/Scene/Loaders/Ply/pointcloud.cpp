#include "pointcloud.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace DirectX;

float PointCloud::Extent() const
{
	return std::max({ boundsMax.x - boundsMin.x, boundsMax.y - boundsMin.y, boundsMax.z - boundsMin.z });
}

float PointCloud::AverageSpacing() const
{
	if (points.size() < 2)
		return 0.01f;

	// Points spread over a volume: spacing scales with the cube root of the count.
	const float volumeSide = std::max(Extent(), 1e-4f);
	const float perAxis = std::cbrt(static_cast<float>(points.size()));
	return volumeSide / std::max(perAxis, 1.0f);
}

void PointCloud::ComputeBounds()
{
	if (points.empty())
	{
		boundsMin = XMFLOAT3(0.0f, 0.0f, 0.0f);
		boundsMax = XMFLOAT3(0.0f, 0.0f, 0.0f);
		return;
	}

	constexpr float largest = std::numeric_limits<float>::max();
	boundsMin = XMFLOAT3(largest, largest, largest);
	boundsMax = XMFLOAT3(-largest, -largest, -largest);

	for (const PointCloudPoint& point : points)
	{
		boundsMin.x = std::min(boundsMin.x, point.position.x);
		boundsMin.y = std::min(boundsMin.y, point.position.y);
		boundsMin.z = std::min(boundsMin.z, point.position.z);
		boundsMax.x = std::max(boundsMax.x, point.position.x);
		boundsMax.y = std::max(boundsMax.y, point.position.y);
		boundsMax.z = std::max(boundsMax.z, point.position.z);
	}
}
