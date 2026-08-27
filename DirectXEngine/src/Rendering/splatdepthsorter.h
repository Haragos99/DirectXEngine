#pragma once
#include <DirectXMath.h>
#include <cstdint>
#include <vector>

// Produces the back-to-front draw order that Gaussian splats need for correct
// alpha blending.
//
// A 16 bit counting sort keeps the cost linear in the splat count (std::sort
// would be far too slow for a million splats), and the order is only rebuilt
// when the view actually moved enough to change it.
class SplatDepthSorter
{
public:
	// Returns true when `order` was rebuilt and has to be re-uploaded.
	bool Update(
		const std::vector<DirectX::XMFLOAT3>& centers,
		const DirectX::XMMATRIX& worldView,
		float sceneExtent,
		std::vector<std::uint32_t>& order);

private:
	bool ViewChanged(const DirectX::XMMATRIX& worldView, float sceneExtent);

	std::vector<float> depths;
	std::vector<std::uint16_t> keys;
	std::vector<std::uint32_t> histogram;
	DirectX::XMFLOAT3 lastDirection = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 lastOrigin = { 0.0f, 0.0f, 0.0f };
	bool sorted = false;
};
