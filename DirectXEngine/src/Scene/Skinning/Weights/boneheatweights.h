#pragma once
#include "closestboneweights.h"
#include "iweightsolver.h"

// Bone heat diffusion, following the reference implementation: start from the
// rigid closest-bone binding and relax it over the surface by solving
// (-L + alpha*D) w_k = alpha*D * ~w_k once per bone, with L the cotangent
// Laplacian and D the inverse squared distance to the closest bone.
//
// Alpha trades the two sides off: large values pin the result to the rigid
// binding, small ones let the heat spread further across the surface.
//
// Eigen stays inside the .cpp, so including this costs nothing.
class BoneHeatWeights : public IWeightSolver
{
public:
	// The epsilon the reference implementation defaults to. Alpha must stay
	// above zero: -L alone is singular.
	static constexpr float kDefaultAlpha = 0.001f;
	static constexpr float kMinAlpha = 0.0001f;
	static constexpr float kMaxAlpha = 10.0f;

	BoneHeatWeights();

	const char* GetName() const override { return "Bone Heat"; }
	std::span<SolverParameter> GetParameters() override { return parameters; }

	// Leaves the closest-bone binding in `weights` whenever it returns
	// anything but Ok, so the mesh still deforms.
	WeightSolveStatus Solve(const SkinBindContext& context, SkinWeightTable& weights) override;

private:
	ClosestBoneWeights seed;
	SolverParameter parameters[1];
};
