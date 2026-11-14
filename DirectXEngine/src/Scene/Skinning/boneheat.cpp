#include "boneheat.h"

#include "halfedgemesh.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	constexpr double kPi = 3.141592653589793;
	constexpr double kHalfPi = 1.5707963267948966;
	// A vertex sitting exactly on a bone would otherwise divide by zero.
	constexpr double kMinimumDistance = 1e-6;
	// A sliver triangle drives its cotangent towards infinity, which would swamp
	// every other entry and leave the solve returning noise.
	constexpr double kMinimumAngle = 1e-4;
	// Below this a row carries no usable influence and is better left as it was.
	constexpr double kMinimumWeightSum = 1e-8;

	// Cotangent of the angle facing `halfEdge` inside its own triangle, written
	// as tan(pi/2 - theta) the way the reference implementation has it.
	double OppositeCotangent(const HalfEdgeMesh& mesh, int halfEdge)
	{
		// A boundary half-edge has no triangle, so there is no angle to take.
		if (halfEdge == HalfEdgeMesh::kInvalid || mesh.IsBoundary(halfEdge))
			return 0.0;

		const double theta = std::clamp(mesh.SectorAngle(mesh.Next(halfEdge)),
		                                kMinimumAngle, kPi - kMinimumAngle);
		return std::tan(kHalfPi - theta);
	}

	// Bone heat only re-decides which joints drive a vertex, so each row has to
	// come back out a partition of unity: a row summing to anything else scales
	// the vertex even where the rig has not moved.
	void Normalise(SkinWeightTable& welded, const SkinWeightTable& rigid)
	{
		for (size_t v = 0; v < welded.VertexCount(); ++v)
		{
			std::vector<float>& row = welded[v];

			double total = 0.0;
			for (float& weight : row)
			{
				// The cotangent Laplacian can overshoot, and a joint pulling a
				// vertex backwards is not something the solve is asking for.
				weight = std::max(weight, 0.0f);
				total += weight;
			}

			if (total < kMinimumWeightSum)
			{
				row = rigid[v];
				continue;
			}

			for (float& weight : row)
				weight = static_cast<float>(weight / total);
		}
	}
}

Eigen::SparseMatrix<double> BoneHeat::CreateLaplaceMatrix(const HalfEdgeMesh& mesh) const
{
	const int n = static_cast<int>(mesh.VertexCount());

	std::vector<Eigen::Triplet<double>> entries;
	std::vector<double> rowSum(n, 0.0);
	for (int v = 0; v < n; ++v)
	{
		for (const int neighbour : mesh.Neighbours(v))
		{
			if (neighbour == v)
				continue;

			const int halfEdge = mesh.FindHalfEdge(v, neighbour);
			if (halfEdge == HalfEdgeMesh::kInvalid)
				continue;

			// Mean of the cotangents of the two angles facing the edge.
			const double weight = (OppositeCotangent(mesh, halfEdge)
			                    + OppositeCotangent(mesh, mesh.Opposite(halfEdge))) / 2.0;
			entries.emplace_back(v, neighbour, weight);
			rowSum[v] += weight;
		}
	}

	// Closing every row to zero is what makes L annihilate constants, and in turn
	// what keeps the solved weights summing to one.
	for (int v = 0; v < n; ++v)
		entries.emplace_back(v, v, -rowSum[v]);

	Eigen::SparseMatrix<double> laplace(n, n);
	laplace.setFromTriplets(entries.begin(), entries.end());
	laplace.makeCompressed();
	return laplace;
}

Eigen::SparseMatrix<double> BoneHeat::CreateDiagonalMatrix(const std::vector<double>& closestDistance) const
{
	const int n = static_cast<int>(closestDistance.size());

	Eigen::SparseMatrix<double> diagonal(n, n);
	diagonal.reserve(Eigen::VectorXi::Constant(n, 1));
	for (int v = 0; v < n; ++v)
	{
		const double distance = std::max(closestDistance[v], kMinimumDistance);
		diagonal.insert(v, v) = 1.0 / (distance * distance);
	}
	diagonal.makeCompressed();

	diagonal *= alpha;
	return diagonal;
}

void BoneHeat::CalculateOptimalWeights(const Eigen::SparseMatrix<double>& diagonal,
                                       const Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>>& solver,
                                       SkinWeightTable& welded) const
{
	const int n = static_cast<int>(welded.VertexCount());
	for (size_t joint = 0; joint < welded.JointCount(); ++joint)
	{
		Eigen::VectorXd baseWeight = Eigen::VectorXd::Zero(n);
		bool influences = false;
		for (int v = 0; v < n; ++v)
		{
			baseWeight[v] = welded[v][joint];
			influences = influences || baseWeight[v] != 0.0;
		}

		// A joint nothing binds to solves to zero, so there is nothing to gain.
		if (!influences)
			continue;

		const Eigen::VectorXd optimal = solver.solve(diagonal * baseWeight);
		if (solver.info() != Eigen::Success || !optimal.allFinite())
			continue;

		for (int v = 0; v < n; ++v)
			welded[v][joint] = static_cast<float>(optimal[v]);
	}
}

bool BoneHeat::Execute(const HalfEdgeMesh& mesh,
                       const std::vector<float>& closestDistance,
                       SkinWeightTable& weights) const
{
	const size_t n = mesh.VertexCount();
	if (n == 0 || weights.IsEmpty()
		|| weights.VertexCount() != mesh.RenderVertexCount()
		|| closestDistance.size() != weights.VertexCount())
	{
		return false;
	}

	// The Laplacian lives on the welded topology while the weights live on the
	// render vertices, so gather down onto the welded ones, solve, scatter back.
	SkinWeightTable welded;
	welded.Resize(n, weights.JointCount());
	std::vector<double> distance(n, std::numeric_limits<double>::infinity());
	for (size_t v = 0; v < weights.VertexCount(); ++v)
	{
		const int target = mesh.VertexOf(v);
		welded[target] = weights[v];
		distance[target] = std::min(distance[target], static_cast<double>(closestDistance[v]));
	}

	const Eigen::SparseMatrix<double> laplace = CreateLaplaceMatrix(mesh);
	const Eigen::SparseMatrix<double> diagonal = CreateDiagonalMatrix(distance);
	const Eigen::SparseMatrix<double> system = -laplace + diagonal;

	Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
	solver.compute(system);
	if (solver.info() != Eigen::Success)
		return false;

	const SkinWeightTable rigid = welded;
	CalculateOptimalWeights(diagonal, solver, welded);
	Normalise(welded, rigid);

	for (size_t v = 0; v < weights.VertexCount(); ++v)
		weights[v] = welded[mesh.VertexOf(v)];

	return true;
}
