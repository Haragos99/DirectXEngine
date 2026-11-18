#pragma once
#include <DirectXMath.h>
#include <vector>

// Dense scalar volume sampled on a regular lattice: the only input marching
// cubes understands. It is a plain container, so any voxel source - an implicit
// field, a scan, a splat cloud - can fill it.
class VoxelGrid
{
public:
	VoxelGrid() = default;
	VoxelGrid(int sizeX, int sizeY, int sizeZ,
	          const DirectX::XMFLOAT3& origin,
	          const DirectX::XMFLOAT3& cellSize);

	int SizeX() const { return size[0]; }
	int SizeY() const { return size[1]; }
	int SizeZ() const { return size[2]; }
	const DirectX::XMFLOAT3& GetOrigin() const { return origin; }
	const DirectX::XMFLOAT3& GetCellSize() const { return cellSize; }

	// True when the grid is too small to hold a single cell.
	bool IsEmpty() const;

	float ValueAt(int x, int y, int z) const;
	void SetValue(int x, int y, int z, float value);

	DirectX::XMFLOAT3 PositionAt(int x, int y, int z) const;

	// Central difference gradient, clamped at the borders. It points towards
	// denser samples, so the outward surface normal is its negation.
	DirectX::XMFLOAT3 GradientAt(int x, int y, int z) const;

private:
	size_t Index(int x, int y, int z) const;
	int Clamp(int value, int axis) const;

	int size[3] = { 0, 0, 0 };
	DirectX::XMFLOAT3 origin = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 cellSize = { 1.0f, 1.0f, 1.0f };
	std::vector<float> values;
};
