#pragma once
#include "iscalarfield.h"

// Implicit sphere: the sample is the signed distance to the surface, positive
// inside. The demo volume marching cubes is exercised with.
class SphereField : public IScalarField
{
public:
	SphereField(const DirectX::XMFLOAT3& center, float radius);

	const char* GetName() const override { return "Sphere"; }
	float Sample(const DirectX::XMFLOAT3& point) const override;
	void GetBounds(DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax) const override;

private:
	DirectX::XMFLOAT3 center;
	float radius;
};
