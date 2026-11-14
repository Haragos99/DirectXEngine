#include "closestboneweights.h"

#include <cmath>
#include <limits>

using namespace DirectX;

namespace
{
	// Enough points that a long bone still wins over a short one nearby.
	constexpr int kSamplesPerBone = 16;

	// Points along every bone, one list per influence.
	std::vector<std::vector<XMFLOAT3>> SampleBones(const std::vector<BoneSegment>& bones)
	{
		std::vector<std::vector<XMFLOAT3>> samples(bones.size());
		for (size_t i = 0; i < bones.size(); ++i)
		{
			const XMVECTOR from = XMLoadFloat3(&bones[i].start);
			const XMVECTOR to = XMLoadFloat3(&bones[i].end);

			// A root fallback is a single point.
			if (XMVector3Equal(from, to))
			{
				samples[i].push_back(bones[i].start);
				continue;
			}

			samples[i].reserve(kSamplesPerBone + 1);
			for (int s = 0; s <= kSamplesPerBone; ++s)
			{
				XMFLOAT3 point;
				XMStoreFloat3(&point, XMVectorLerp(from, to, static_cast<float>(s) / kSamplesPerBone));
				samples[i].push_back(point);
			}
		}
		return samples;
	}
}

WeightSolveStatus ClosestBoneWeights::Solve(const SkinBindContext& context, SkinWeightTable& out)
{
	std::vector<float> unused;
	return Solve(context, out, unused);
}

WeightSolveStatus ClosestBoneWeights::Solve(const SkinBindContext& context, SkinWeightTable& out,
                                            std::vector<float>& closestDistance)
{
	const std::vector<VertexData>& vertices = context.restVertices;
	out.Resize(vertices.size(), context.bones.size());
	closestDistance.assign(vertices.size(), 0.0f);

	if (context.bones.empty())
		return WeightSolveStatus::NoInfluences;

	const std::vector<std::vector<XMFLOAT3>> bones = SampleBones(context.bones);
	const XMMATRIX meshBindToRig = XMLoadFloat4x4(&context.meshBindToRig);

	for (size_t v = 0; v < vertices.size(); ++v)
	{
		const XMVECTOR position = XMVector3Transform(XMLoadFloat3(&vertices[v].position), meshBindToRig);

		size_t closestInfluence = 0;
		float closest = std::numeric_limits<float>::infinity();
		for (size_t i = 0; i < bones.size(); ++i)
		{
			for (const XMFLOAT3& point : bones[i])
			{
				const float distance = XMVectorGetX(
					XMVector3LengthSq(XMVectorSubtract(XMLoadFloat3(&point), position)));
				if (distance < closest)
				{
					closest = distance;
					closestInfluence = i;
				}
			}
		}

		out[v][closestInfluence] = 1.0f;
		if (std::isfinite(closest))
			closestDistance[v] = std::sqrt(closest);
	}
	return WeightSolveStatus::Ok;
}
