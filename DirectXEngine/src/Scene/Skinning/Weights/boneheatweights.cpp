#include "boneheatweights.h"

#include "halfedgemesh.h"

#include <Eigen/Sparse>

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

	// Bone heat only re-decides which bones drive a vertex, so each row has to
	// come back out a partition of unity: a row summing to anything else scales
	// the vertex even where the rig has not moved.
	void Normalise(SkinWeightTable& welded, const SkinWeightTable& rigid)
	{
		for (size_t v = 0; v < welded.VertexCount(); ++v)
		{
			const std::span<float> row = welded[v];

			double total = 0.0;
			for (float& weight : row)
			{
				// The cotangent Laplacian can overshoot, and a bone pulling a
				// vertex backwards is not something the solve is asking for.
				weight = std::max(weight, 0.0f);
				total += weight;
			}

			if (total < kMinimumWeightSum)
			{
				welded.CopyRow(v, rigid, v);
				continue;
			}

			for (float& weight : row)
				weight = static_cast<float>(weight / total);
		}
	}
}

// Cotangent Laplacian of the welded mesh (BoneHeat::createLaplaceMatrix).
static Eigen::SparseMatrix<double> CreateLaplaceMatrix(const HalfEdgeMesh& mesh)
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

// alpha / d^2 per welded vertex (BoneHeat::createDiagolaleMatrix).
static Eigen::SparseMatrix<double> CreateDiagonalMatrix(const std::vector<double>& closestDistance, double alpha)
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

// One solve per influence (BoneHeat::calculateOptimalWeights).
static void CalculateOptimalWeights(const Eigen::SparseMatrix<double>& diagonal,
                                    const Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>>& solver,
                                    SkinWeightTable& welded)
{
	const int n = static_cast<int>(welded.VertexCount());
	for (size_t influence = 0; influence < welded.InfluenceCount(); ++influence)
	{
		Eigen::VectorXd baseWeight = Eigen::VectorXd::Zero(n);
		bool used = false;
		for (int v = 0; v < n; ++v)
		{
			baseWeight[v] = welded[v][influence];
			used = used || baseWeight[v] != 0.0;
		}

		// An influence nothing binds to solves to zero, so there is nothing to gain.
		if (!used)
			continue;

		const Eigen::VectorXd optimal = solver.solve(diagonal * baseWeight);
		if (solver.info() != Eigen::Success || !optimal.allFinite())
			continue;

		for (int v = 0; v < n; ++v)
			welded[v][influence] = static_cast<float>(optimal[v]);
	}
}

BoneHeatWeights::BoneHeatWeights()
	: parameters{ { "Alpha", kDefaultAlpha, kMinAlpha, kMaxAlpha,
	                "How hard the solve is pulled back to the closest-bone weights.\n"
	                "Lower spreads the influence further over the surface.", true } }
{
}

WeightSolveStatus BoneHeatWeights::Solve(const SkinBindContext& context, SkinWeightTable& weights)
{
	// The rigid binding is both the seed and what is left behind on failure.
	std::vector<float> closestDistance;
	const WeightSolveStatus seeded = seed.Solve(context, weights, closestDistance);
	if (seeded != WeightSolveStatus::Ok)
		return seeded;

	// Without connectivity there is no surface to diffuse over.
	if (context.topology == nullptr || context.topology->IsEmpty())
		return WeightSolveStatus::NoTopology;

	const HalfEdgeMesh& mesh = *context.topology;
	const size_t n = mesh.VertexCount();
	if (weights.VertexCount() != mesh.RenderVertexCount())
		return WeightSolveStatus::SizeMismatch;

	const double alpha = std::clamp(static_cast<double>(parameters[0].value),
	                                static_cast<double>(kMinAlpha), static_cast<double>(kMaxAlpha));

	// The Laplacian lives on the welded topology while the weights live on the
	// render vertices, so gather down onto the welded ones, solve, scatter back.
	SkinWeightTable welded;
	welded.Resize(n, weights.InfluenceCount());
	std::vector<double> distance(n, std::numeric_limits<double>::infinity());
	for (size_t v = 0; v < weights.VertexCount(); ++v)
	{
		const int target = mesh.VertexOf(v);
		welded.CopyRow(target, weights, v);
		distance[target] = std::min(distance[target], static_cast<double>(closestDistance[v]));
	}

	const Eigen::SparseMatrix<double> laplace = CreateLaplaceMatrix(mesh);
	const Eigen::SparseMatrix<double> diagonal = CreateDiagonalMatrix(distance, alpha);
	const Eigen::SparseMatrix<double> system = -laplace + diagonal;

	Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
	solver.compute(system);
	if (solver.info() != Eigen::Success)
		return WeightSolveStatus::FactorisationFailed;

	const SkinWeightTable rigid = welded;
	CalculateOptimalWeights(diagonal, solver, welded);
	Normalise(welded, rigid);

	for (size_t v = 0; v < weights.VertexCount(); ++v)
		weights.CopyRow(v, welded, mesh.VertexOf(v));

	return WeightSolveStatus::Ok;
}
