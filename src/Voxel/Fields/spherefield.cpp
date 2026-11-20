#include "spherefield.h"

using namespace DirectX;

namespace
{
	// Slack around the sphere so the outermost cells are fully outside and the
	// extracted surface closes.
	constexpr float kBoundsPadding = 1.25f;
}

SphereField::SphereField(const XMFLOAT3& _center, float _radius)
	: center(_center), radius(_radius)
{
}

float SphereField::Sample(const XMFLOAT3& point) const
{
	const XMVECTOR delta = XMVectorSubtract(XMLoadFloat3(&point), XMLoadFloat3(&center));
	return radius - XMVectorGetX(XMVector3Length(delta));
}

void SphereField::GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const
{
	const float extent = radius * kBoundsPadding;
	outMin = XMFLOAT3(center.x - extent, center.y - extent, center.z - extent);
	outMax = XMFLOAT3(center.x + extent, center.y + extent, center.z + extent);
}
