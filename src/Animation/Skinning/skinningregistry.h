#pragma once
#include "iskindeformer.h"
#include "iweightsolver.h"

#include <memory>
#include <span>
#include <wrl/client.h>

struct WeightSolverEntry
{
	const char* name;
	std::unique_ptr<IWeightSolver> (*create)();
};

struct SkinDeformerEntry
{
	const char* name;
	std::unique_ptr<ISkinDeformer> (*create)(Microsoft::WRL::ComPtr<ID3D11Device>,
	                                         Microsoft::WRL::ComPtr<ID3D11DeviceContext>);
};

namespace SkinningRegistry
{
	std::span<const WeightSolverEntry> WeightSolvers();
	std::span<const SkinDeformerEntry> Deformers();
}
