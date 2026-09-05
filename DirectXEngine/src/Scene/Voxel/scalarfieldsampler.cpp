#include "scalarfieldsampler.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

VoxelGrid ScalarFieldSampler::Sample(const IScalarField& field, int resolution, const ProgressCallback& onProgress)
{
	resolution = std::max(resolution, 2);

	XMFLOAT3 minCorner;
	XMFLOAT3 maxCorner;
	field.GetBounds(minCorner, maxCorner);

	const XMFLOAT3 extent(maxCorner.x - minCorner.x,
	                      maxCorner.y - minCorner.y,
	                      maxCorner.z - minCorner.z);
	const float longest = std::max({ extent.x, extent.y, extent.z });
	if (longest <= 0.0f)
		return VoxelGrid();

	const float step = longest / (resolution - 1);
	const auto samplesAlong = [step](float length) {
		return std::max(2, static_cast<int>(std::ceil(length / step)) + 1);
	};

	VoxelGrid grid(samplesAlong(extent.x), samplesAlong(extent.y), samplesAlong(extent.z),
	               minCorner, XMFLOAT3(step, step, step));

	for (int z = 0; z < grid.SizeZ(); ++z)
	{
		for (int y = 0; y < grid.SizeY(); ++y)
		{
			for (int x = 0; x < grid.SizeX(); ++x)
			{
				grid.SetValue(x, y, z, field.Sample(grid.PositionAt(x, y, z)));
			}
		}

		if (onProgress)
			onProgress(static_cast<float>(z + 1) / grid.SizeZ());
	}

	return grid;
}
