#pragma once
#include "iweightsolver.h"

#include <vector>

class ClosestBoneWeights : public IWeightSolver
{
public:
	const char* GetName() const override { return "Default (closest bone)"; }

	WeightSolveStatus Solve(const SkinBindContext& context, SkinWeightTable& out) override;

	WeightSolveStatus Solve(const SkinBindContext& context, SkinWeightTable& out,
	                        std::vector<float>& closestDistance);
};
