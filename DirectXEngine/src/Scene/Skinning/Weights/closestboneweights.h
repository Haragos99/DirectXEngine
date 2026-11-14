#pragma once
#include "iweightsolver.h"

#include <vector>

// The closest bone takes the whole vertex, as in the reference implementation
// (Skinning::calculateSkinning). Rigid, so every transition is a hard edge;
// Bone Heat starts from it.
class ClosestBoneWeights : public IWeightSolver
{
public:
	const char* GetName() const override { return "Default (closest bone)"; }

	WeightSolveStatus Solve(const SkinBindContext& context, SkinWeightTable& out) override;

	// Same as Solve, and also hands back each render vertex's distance to the
	// bone that won it, which Bone Heat weighs its constraint by.
	WeightSolveStatus Solve(const SkinBindContext& context, SkinWeightTable& out,
	                        std::vector<float>& closestDistance);
};
