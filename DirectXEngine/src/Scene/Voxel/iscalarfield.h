#pragma once
#include <DirectXMath.h>

// A density function that marching cubes turns into a surface. Values above the
// iso level are inside the volume, values below it are outside.
//
// This is the seam that keeps the algorithm reusable: an implicit primitive
// today, a Gaussian splat density or an imported voxel volume tomorrow, without
// touching the mesher.
class IScalarField
{
public:
	virtual ~IScalarField() = default;

	// Name used for the generated scene object.
	virtual const char* GetName() const = 0;

	// Density at a point in the field's own space.
	virtual float Sample(const DirectX::XMFLOAT3& point) const = 0;

	// Axis aligned region worth sampling; everything outside is empty space.
	virtual void GetBounds(DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax) const = 0;
};
