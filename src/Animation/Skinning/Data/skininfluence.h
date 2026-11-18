#pragma once
#include <DirectXMath.h>
#include <string>

struct SkinInfluence
{
	static constexpr int kRootFallback = -1;

	int bone = kRootFallback;
	int drivingJoint = 0;
	DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
	std::string name;
};
