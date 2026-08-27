#pragma once
#include <DirectXMath.h>
#include <vector>

// A plain coloured point, the whole payload of a regular (non Gaussian) PLY
// point cloud: no covariance, just a position and a colour.
struct PointCloudPoint
{
	DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

struct PointCloud
{
	std::vector<PointCloudPoint> points;
	DirectX::XMFLOAT3 boundsMin{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 boundsMax{ 0.0f, 0.0f, 0.0f };

	bool Empty() const { return points.empty(); }
	// Largest bounding box side.
	float Extent() const;
	// Rough average distance between neighbouring points, used to pick a
	// splat radius that closes the gaps without blurring detail away.
	float AverageSpacing() const;
	void ComputeBounds();
};
