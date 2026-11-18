#include "skinningregistry.h"

#include "boneheatweights.h"
#include "closestboneweights.h"
#include "dualquaternionskinning.h"
#include "linearblendskinning.h"

using Microsoft::WRL::ComPtr;

namespace
{
	template <typename T>
	std::unique_ptr<IWeightSolver> MakeSolver()
	{
		return std::make_unique<T>();
	}

	template <typename T>
	std::unique_ptr<ISkinDeformer> MakeDeformer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context)
	{
		return std::make_unique<T>(device, context);
	}

	const WeightSolverEntry kWeightSolvers[] = {
		{ "Default (closest bone)", &MakeSolver<ClosestBoneWeights> },
		{ "Bone Heat", &MakeSolver<BoneHeatWeights> },
	};

	const SkinDeformerEntry kDeformers[] = {
		{ "Linear Blend (LBS)", &MakeDeformer<LinearBlendSkinning> },
		{ "Dual Quaternion (DQS)", &MakeDeformer<DualQuaternionSkinning> },
	};
}

std::span<const WeightSolverEntry> SkinningRegistry::WeightSolvers()
{
	return kWeightSolvers;
}

std::span<const SkinDeformerEntry> SkinningRegistry::Deformers()
{
	return kDeformers;
}
