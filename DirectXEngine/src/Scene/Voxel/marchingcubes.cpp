#include "marchingcubes.h"
#include "marchingcubestables.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;
using namespace MarchingCubesTables;

namespace
{
	// Below this the two corner samples are treated as equal and the crossing is
	// put in the middle of the edge.
	constexpr float kDenominatorEpsilon = 1e-6f;

	XMFLOAT2 SphericalTexcoord(const XMFLOAT3& normal)
	{
		const float u = std::atan2(normal.z, normal.x) / (2.0f * XM_PI) + 0.5f;
		const float v = std::acos(std::clamp(normal.y, -1.0f, 1.0f)) / XM_PI;
		return XMFLOAT2(u, v);
	}
}

MeshData MarchingCubes::Generate(const VoxelGrid& grid, float isoLevel, const ProgressCallback& onProgress) const
{
	MeshData out;
	if (grid.IsEmpty())
		return out;

	// Cells on slice z only ever own edges on grid slices z and z+1, so two
	// slices of vertex slots are enough to weld the surface. A cache covering
	// the whole volume would cost more memory than the grid itself.
	const size_t sliceSlots = static_cast<size_t>(grid.SizeX()) * grid.SizeY() * 3;
	std::vector<int> cache(sliceSlots * 2, -1);

	for (int z = 0; z < grid.SizeZ() - 1; ++z)
	{
		for (int y = 0; y < grid.SizeY() - 1; ++y)
		{
			for (int x = 0; x < grid.SizeX() - 1; ++x)
			{
				int cubeIndex = 0;
				for (int corner = 0; corner < 8; ++corner)
				{
					const int* offset = kCornerOffset[corner];
					const float value = grid.ValueAt(x + offset[0], y + offset[1], z + offset[2]);
					if (value < isoLevel)
						cubeIndex |= 1 << corner;
				}

				// Fully inside or fully outside: no surface in this cell.
				if (kEdgeTable[cubeIndex] == 0)
					continue;

				const int* triangles = kTriangleTable[cubeIndex];
				for (int i = 0; triangles[i] != -1; i += 3)
				{
					const UINT a = EdgeVertex(grid, isoLevel, x, y, z, triangles[i], cache, out);
					const UINT b = EdgeVertex(grid, isoLevel, x, y, z, triangles[i + 1], cache, out);
					const UINT c = EdgeVertex(grid, isoLevel, x, y, z, triangles[i + 2], cache, out);

					// Two corners landing on the same crossing collapse the triangle.
					if (a == b || b == c || a == c)
						continue;

					// Reversed against the table to match the winding the rest of
					// the engine feeds the rasterizer.
					out.indices.push_back(a);
					out.indices.push_back(c);
					out.indices.push_back(b);
					out.faces.emplace_back(a, c, b);
				}
			}
		}

		if (onProgress)
			onProgress(static_cast<float>(z + 1) / (grid.SizeZ() - 1));

		// Slice z+1 becomes the near slice of the next cell slice; the far one
		// starts empty again.
		std::copy(cache.begin() + sliceSlots, cache.end(), cache.begin());
		std::fill(cache.begin() + sliceSlots, cache.end(), -1);
	}

	out.BuildVertices();
	return out;
}

UINT MarchingCubes::EdgeVertex(const VoxelGrid& grid, float isoLevel,
                               int x, int y, int z, int edge,
                               std::vector<int>& cache, MeshData& out)
{
	const int* owner = kEdgeOwner[edge];
	const size_t slot = ((static_cast<size_t>(owner[2]) * grid.SizeY() + (y + owner[1]))
	                     * grid.SizeX() + (x + owner[0])) * 3 + owner[3];
	if (cache[slot] >= 0)
		return static_cast<UINT>(cache[slot]);

	const int* corners = kEdgeCorners[edge];
	const int* offsetA = kCornerOffset[corners[0]];
	const int* offsetB = kCornerOffset[corners[1]];
	const int ax = x + offsetA[0], ay = y + offsetA[1], az = z + offsetA[2];
	const int bx = x + offsetB[0], by = y + offsetB[1], bz = z + offsetB[2];

	const float valueA = grid.ValueAt(ax, ay, az);
	const float valueB = grid.ValueAt(bx, by, bz);
	const float denominator = valueB - valueA;
	const float t = std::abs(denominator) > kDenominatorEpsilon
		? std::clamp((isoLevel - valueA) / denominator, 0.0f, 1.0f)
		: 0.5f;

	const XMFLOAT3 positionA = grid.PositionAt(ax, ay, az);
	const XMFLOAT3 positionB = grid.PositionAt(bx, by, bz);
	XMFLOAT3 position;
	XMStoreFloat3(&position, XMVectorLerp(XMLoadFloat3(&positionA), XMLoadFloat3(&positionB), t));

	// The gradient climbs towards denser samples, so the outward normal is its
	// opposite. Interpolating it along the edge keeps the shading smooth.
	const XMFLOAT3 gradientA = grid.GradientAt(ax, ay, az);
	const XMFLOAT3 gradientB = grid.GradientAt(bx, by, bz);
	const XMVECTOR gradient = XMVectorLerp(XMLoadFloat3(&gradientA), XMLoadFloat3(&gradientB), t);
	XMFLOAT3 normal(0.0f, 1.0f, 0.0f);
	if (XMVectorGetX(XMVector3LengthSq(gradient)) > kDenominatorEpsilon)
		XMStoreFloat3(&normal, XMVectorNegate(XMVector3Normalize(gradient)));

	const UINT index = static_cast<UINT>(out.positions.size());
	out.positions.push_back(position);
	out.normals.push_back(normal);
	out.texcoords.push_back(SphericalTexcoord(normal));
	cache[slot] = static_cast<int>(index);
	return index;
}
