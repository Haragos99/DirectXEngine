#include "splatdensityvolume.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace DirectX;

namespace
{
	// Splats fainter than one 8 bit alpha step cannot lift a cell over any
	// useful iso level.
	constexpr float kMinOpacity = 1.0f / 255.0f;

	// A Gaussian thinner than the grid would fall between two samples, so its
	// deviation is floored to this fraction of a cell.
	constexpr float kMinDeviationInCells = 0.5f;

	// A single background Gaussian can be as wide as the whole capture. Letting
	// it scatter would cost more than the rest of the cloud together and would
	// only smear the surface, so anything reaching further is dropped.
	constexpr int kMaxCellSpan = 64;

	// Empty rim of cells so splats sitting on the bounding box still close.
	constexpr int kBorderCells = 2;

	// Splats per progress report. Frequent enough for a smooth bar, rare enough
	// that the callback does not show up in the profile.
	constexpr size_t kProgressInterval = 4096;
}

SplatDensityVolume::SplatDensityVolume(std::vector<SplatInstance> _splats,
                                       const XMFLOAT3& _boundsMin,
                                       const XMFLOAT3& _boundsMax,
                                       float _reachInDeviations,
                                       const Transform& _placement,
                                       std::string sourceName)
	: splats(std::move(_splats))
	, boundsMin(_boundsMin)
	, boundsMax(_boundsMax)
	, reachInDeviations((std::max)(_reachInDeviations, 0.5f))
	, placement(_placement)
	, name(std::move(sourceName))
{
}

VoxelGrid SplatDensityVolume::Build(int resolution, const ProgressCallback& onProgress) const
{
	resolution = (std::max)(resolution, 2);

	const XMFLOAT3 extent(boundsMax.x - boundsMin.x,
	                      boundsMax.y - boundsMin.y,
	                      boundsMax.z - boundsMin.z);
	const float longest = (std::max)({ extent.x, extent.y, extent.z });
	if (longest <= 0.0f || splats.empty())
		return VoxelGrid();

	const float step = longest / (resolution - 1);
	const XMFLOAT3 origin(boundsMin.x - kBorderCells * step,
	                      boundsMin.y - kBorderCells * step,
	                      boundsMin.z - kBorderCells * step);
	const auto samplesAlong = [step](float length) {
		return (std::max)(2, static_cast<int>(std::ceil(length / step)) + 1 + 2 * kBorderCells);
	};

	VoxelGrid grid(samplesAlong(extent.x), samplesAlong(extent.y), samplesAlong(extent.z),
	               origin, XMFLOAT3(step, step, step));

	for (size_t i = 0; i < splats.size(); ++i)
	{
		Scatter(splats[i], grid);

		if (onProgress && (i % kProgressInterval) == 0)
			onProgress(static_cast<float>(i) / splats.size());
	}

	if (onProgress)
		onProgress(1.0f);

	return grid;
}

void SplatDensityVolume::Scatter(const SplatInstance& splat, VoxelGrid& grid) const
{
	const float opacity = splat.color.w;
	if (opacity <= kMinOpacity)
		return;

	const XMFLOAT3& cell = grid.GetCellSize();
	const XMFLOAT3 deviation((std::max)(splat.scale.x, cell.x * kMinDeviationInCells),
	                         (std::max)(splat.scale.y, cell.y * kMinDeviationInCells),
	                         (std::max)(splat.scale.z, cell.z * kMinDeviationInCells));

	const float reach = reachInDeviations * (std::max)({ deviation.x, deviation.y, deviation.z });
	if (reach > kMaxCellSpan * 0.5f * cell.x)
		return;

	const XMFLOAT3& origin = grid.GetOrigin();
	const int size[3] = { grid.SizeX(), grid.SizeY(), grid.SizeZ() };
	const float center[3] = { splat.center.x, splat.center.y, splat.center.z };
	const float originAxis[3] = { origin.x, origin.y, origin.z };
	const float cellAxis[3] = { cell.x, cell.y, cell.z };

	int lower[3];
	int upper[3];
	for (int axis = 0; axis < 3; ++axis)
	{
		const float relative = center[axis] - originAxis[axis];
		lower[axis] = (std::max)(0, static_cast<int>(std::floor((relative - reach) / cellAxis[axis])));
		upper[axis] = (std::min)(size[axis] - 1, static_cast<int>(std::ceil((relative + reach) / cellAxis[axis])));
		if (lower[axis] > upper[axis])
			return;
	}

	const XMVECTOR splatCenter = XMLoadFloat3(&splat.center);
	const XMVECTOR rotation = XMLoadFloat4(&splat.rotation);
	const XMVECTOR inverseDeviation = XMVectorReciprocal(XMLoadFloat3(&deviation));
	const float cutoff = reachInDeviations * reachInDeviations;

	for (int z = lower[2]; z <= upper[2]; ++z)
	{
		for (int y = lower[1]; y <= upper[1]; ++y)
		{
			for (int x = lower[0]; x <= upper[0]; ++x)
			{
				const XMFLOAT3 point = grid.PositionAt(x, y, z);
				// Into the splat's own frame, where the covariance is diagonal
				// and the falloff is a plain squared distance.
				const XMVECTOR offset = XMVectorSubtract(XMLoadFloat3(&point), splatCenter);
				const XMVECTOR local = XMVectorMultiply(XMVector3InverseRotate(offset, rotation), inverseDeviation);
				const float squaredDistance = XMVectorGetX(XMVector3LengthSq(local));
				if (squaredDistance > cutoff)
					continue;

				const float density = opacity * std::exp(-0.5f * squaredDistance);
				if (density > grid.ValueAt(x, y, z))
					grid.SetValue(x, y, z, density);
			}
		}
	}
}
