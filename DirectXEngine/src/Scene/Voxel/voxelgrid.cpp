#include "voxelgrid.h"

#include <algorithm>

using namespace DirectX;

VoxelGrid::VoxelGrid(int sizeX, int sizeY, int sizeZ,
                     const XMFLOAT3& _origin,
                     const XMFLOAT3& _cellSize)
	: origin(_origin), cellSize(_cellSize)
{
	size[0] = std::max(sizeX, 0);
	size[1] = std::max(sizeY, 0);
	size[2] = std::max(sizeZ, 0);
	values.assign(static_cast<size_t>(size[0]) * size[1] * size[2], 0.0f);
}

bool VoxelGrid::IsEmpty() const
{
	return size[0] < 2 || size[1] < 2 || size[2] < 2;
}

size_t VoxelGrid::Index(int x, int y, int z) const
{
	return (static_cast<size_t>(z) * size[1] + y) * size[0] + x;
}

int VoxelGrid::Clamp(int value, int axis) const
{
	return std::clamp(value, 0, size[axis] - 1);
}

float VoxelGrid::ValueAt(int x, int y, int z) const
{
	return values[Index(x, y, z)];
}

void VoxelGrid::SetValue(int x, int y, int z, float value)
{
	values[Index(x, y, z)] = value;
}

XMFLOAT3 VoxelGrid::PositionAt(int x, int y, int z) const
{
	return XMFLOAT3(origin.x + x * cellSize.x,
	                origin.y + y * cellSize.y,
	                origin.z + z * cellSize.z);
}

XMFLOAT3 VoxelGrid::GradientAt(int x, int y, int z) const
{
	const float dx = ValueAt(Clamp(x + 1, 0), y, z) - ValueAt(Clamp(x - 1, 0), y, z);
	const float dy = ValueAt(x, Clamp(y + 1, 1), z) - ValueAt(x, Clamp(y - 1, 1), z);
	const float dz = ValueAt(x, y, Clamp(z + 1, 2)) - ValueAt(x, y, Clamp(z - 1, 2));

	return XMFLOAT3(dx / (2.0f * cellSize.x),
	                dy / (2.0f * cellSize.y),
	                dz / (2.0f * cellSize.z));
}
