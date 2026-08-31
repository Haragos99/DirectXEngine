#include "bone.h"

#include <array>

using namespace DirectX;

namespace
{
	constexpr std::array<XMFLOAT3, 24> kPalette = {
		XMFLOAT3(0.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 1.0f),
		XMFLOAT3(0.5f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.5f, 0.5f), XMFLOAT3(0.5f, 0.5f, 1.0f),
		XMFLOAT3(0.2f, 0.4f, 0.4f), XMFLOAT3(0.7f, 0.3f, 0.0f), XMFLOAT3(0.0f, 0.3f, 0.7f),
		XMFLOAT3(0.0f, 0.7f, 0.3f), XMFLOAT3(0.7f, 0.0f, 0.3f), XMFLOAT3(0.3f, 0.0f, 0.7f),
		XMFLOAT3(0.3f, 0.7f, 0.0f), XMFLOAT3(0.7f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.7f, 0.0f),
		XMFLOAT3(0.0f, 0.0f, 0.7f), XMFLOAT3(0.7f, 0.7f, 0.7f), XMFLOAT3(0.5f, 1.0f, 0.2f),
		XMFLOAT3(1.0f, 0.6f, 0.2f), XMFLOAT3(0.4f, 0.5f, 1.0f), XMFLOAT3(0.1f, 0.5f, 0.5f),
		XMFLOAT3(0.5f, 0.3f, 0.0f), XMFLOAT3(0.1f, 0.3f, 0.7f), XMFLOAT3(0.1f, 0.7f, 0.3f),
	};
}

Bone::Bone(int boneId, int start, int end, const XMFLOAT3& boneColor)
	: id(boneId), startJoint(start), endJoint(end), color(boneColor)
{
}

XMFLOAT3 BoneColors::For(int boneIndex)
{
	const size_t index = static_cast<size_t>(boneIndex < 0 ? 0 : boneIndex) % kPalette.size();
	return kPalette[index];
}
