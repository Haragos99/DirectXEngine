#pragma once
#include "iskindeformer.h"
#include "iweightsolver.h"

#include <memory>
#include <span>
#include <wrl/client.h>

// Every weight solver and deformer the engine offers, in the order the UI
// lists them. Adding a technique means adding one line here; nothing else
// switches on the technique.
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
	// The first entry of each list is the default.
	std::span<const WeightSolverEntry> WeightSolvers();
	std::span<const SkinDeformerEntry> Deformers();
}
