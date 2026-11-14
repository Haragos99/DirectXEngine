#pragma once
#include "skinweights.h"

#include <Eigen/Sparse>
#include <vector>

class HalfEdgeMesh;

// Bone heat diffusion, following the reference implementation: start from the
// rigid closest-bone binding and relax it over the surface by solving
// (-L + alpha*D) w_k = alpha*D * ~w_k once per bone, with L the cotangent
// Laplacian and D the inverse squared distance to the closest bone.
//
// Alpha trades the two sides off: large values pin the result to the rigid
// binding, small ones let the heat spread further across the surface.
class BoneHeat
{
public:
	explicit BoneHeat(float _alpha) : alpha(_alpha) {}

	// `weights` arrives holding the rigid binding and leaves holding the
	// diffused one, still a partition of unity so the mesh itself is untouched.
	// `closestDistance` is per render vertex, measured against the rig's bind pose.
	bool Execute(const HalfEdgeMesh& mesh,
	             const std::vector<float>& closestDistance,
	             SkinWeightTable& weights) const;

private:
	Eigen::SparseMatrix<double> CreateLaplaceMatrix(const HalfEdgeMesh& mesh) const;
	Eigen::SparseMatrix<double> CreateDiagonalMatrix(const std::vector<double>& closestDistance) const;
	void CalculateOptimalWeights(const Eigen::SparseMatrix<double>& diagonal,
	                             const Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>>& solver,
	                             SkinWeightTable& welded) const;

	float alpha;
};
