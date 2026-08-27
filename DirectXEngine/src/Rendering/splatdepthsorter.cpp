#include "splatdepthsorter.h"

#include <algorithm>
#include <cfloat>

using namespace DirectX;

namespace
{
	constexpr std::size_t kBucketCount = 65536;
	constexpr float kMaxKey = 65535.0f;
	// About one degree of rotation, below which the order barely changes.
	constexpr float kDirectionEpsilon = 0.9998f;
}

bool SplatDepthSorter::ViewChanged(const XMMATRIX& worldView, float sceneExtent)
{
	XMFLOAT4X4 matrix;
	XMStoreFloat4x4(&matrix, worldView);

	// Row-vector convention: the third column is the view depth axis and the
	// fourth row is the object origin expressed in view space.
	XMFLOAT3 direction;
	XMStoreFloat3(&direction, XMVector3Normalize(XMVectorSet(matrix._13, matrix._23, matrix._33, 0.0f)));
	const XMFLOAT3 origin(matrix._41, matrix._42, matrix._43);

	const float alignment =
		direction.x * lastDirection.x + direction.y * lastDirection.y + direction.z * lastDirection.z;

	const float dx = origin.x - lastOrigin.x;
	const float dy = origin.y - lastOrigin.y;
	const float dz = origin.z - lastOrigin.z;
	const float moveThreshold = std::max(0.02f * sceneExtent, 1e-3f);

	const bool changed = alignment < kDirectionEpsilon
		|| (dx * dx + dy * dy + dz * dz) > moveThreshold * moveThreshold;

	if (changed)
	{
		lastDirection = direction;
		lastOrigin = origin;
	}

	return changed;
}

bool SplatDepthSorter::Update(
	const std::vector<XMFLOAT3>& centers,
	const XMMATRIX& worldView,
	float sceneExtent,
	std::vector<std::uint32_t>& order)
{
	const std::size_t count = centers.size();
	if (count == 0)
		return false;

	const bool viewMoved = ViewChanged(worldView, sceneExtent);
	if (sorted && order.size() == count && !viewMoved)
		return false;

	XMFLOAT4X4 matrix;
	XMStoreFloat4x4(&matrix, worldView);
	const float zx = matrix._13;
	const float zy = matrix._23;
	const float zz = matrix._33;
	const float zw = matrix._43;

	depths.resize(count);
	float minDepth = FLT_MAX;
	float maxDepth = -FLT_MAX;
	for (std::size_t i = 0; i < count; ++i)
	{
		const XMFLOAT3& center = centers[i];
		const float depth = center.x * zx + center.y * zy + center.z * zz + zw;
		depths[i] = depth;
		minDepth = std::min(minDepth, depth);
		maxDepth = std::max(maxDepth, depth);
	}

	const float range = maxDepth - minDepth;
	const float quantise = range > 1e-6f ? kMaxKey / range : 0.0f;

	// Far splats get the small keys, so ascending key order means back to front.
	keys.resize(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		const float normalised = (depths[i] - minDepth) * quantise;
		const std::uint32_t quantised = static_cast<std::uint32_t>(std::clamp(normalised, 0.0f, kMaxKey));
		keys[i] = static_cast<std::uint16_t>(65535u - quantised);
	}

	histogram.assign(kBucketCount, 0u);
	for (std::size_t i = 0; i < count; ++i)
		++histogram[keys[i]];

	std::uint32_t offset = 0;
	for (std::size_t bucket = 0; bucket < kBucketCount; ++bucket)
	{
		const std::uint32_t bucketSize = histogram[bucket];
		histogram[bucket] = offset;
		offset += bucketSize;
	}

	order.resize(count);
	for (std::size_t i = 0; i < count; ++i)
		order[histogram[keys[i]]++] = static_cast<std::uint32_t>(i);

	sorted = true;
	return true;
}
