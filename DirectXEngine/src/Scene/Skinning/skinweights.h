#pragma once
#include <cstdint>

// Per-vertex joint influences. The layout mirrors SkinWeight in
// LinearBlendSkinningCS.hlsl, so these 32 bytes must not change alone.
struct SkinWeights
{
	static constexpr int kMaxInfluences = 4;

	uint32_t joints[kMaxInfluences]{ 0, 0, 0, 0 };
	float weights[kMaxInfluences]{ 0.0f, 0.0f, 0.0f, 0.0f };
};
