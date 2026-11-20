#pragma once
#include "skinbindcontext.h"
#include "skinweights.h"

#include <span>

enum class WeightSolveStatus
{
	Ok,
	NoInfluences,        // the rig has nothing to bind to
	NoTopology,          // the solver needs connectivity the mesh does not have
	SizeMismatch,        // topology and render vertices disagree
	FactorisationFailed, // the linear system could not be solved
};

inline const char* GetWeightSolveStatusMessage(WeightSolveStatus status)
{
	switch (status)
	{
	case WeightSolveStatus::Ok:                  return "Ok";
	case WeightSolveStatus::NoInfluences:        return "the rig has no joints to bind to";
	case WeightSolveStatus::NoTopology:          return "the mesh has no topology to diffuse over";
	case WeightSolveStatus::SizeMismatch:        return "the topology does not match the vertices";
	case WeightSolveStatus::FactorisationFailed: return "the linear system could not be factorised";
	}
	return "unknown error";
}

struct SolverParameter
{
	const char* name;
	float value;
	float min;
	float max;
	const char* tooltip;
	bool logarithmic = false;
};

class IWeightSolver
{
public:
	virtual ~IWeightSolver() = default;

	virtual const char* GetName() const = 0;
	virtual std::span<SolverParameter> GetParameters() { return {}; }
	std::span<const SolverParameter> GetParameters() const
	{
		return const_cast<IWeightSolver*>(this)->GetParameters();
	}

	virtual WeightSolveStatus Solve(const SkinBindContext& context, SkinWeightTable& out) = 0;
};
