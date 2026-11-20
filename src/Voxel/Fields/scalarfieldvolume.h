#pragma once
#include "iscalarfield.h"
#include "ivoxelsource.h"
#include "scalarfieldsampler.h"

#include <memory>
#include <utility>

// Presents a continuous scalar field as a voxel source, so implicit volumes go
// through the same meshing entry point as sampled ones.
class ScalarFieldVolume : public IVoxelSource
{
public:
	explicit ScalarFieldVolume(std::shared_ptr<IScalarField> _field) : field(std::move(_field)) {}

	const char* GetName() const override { return field->GetName(); }
	VoxelGrid Build(int resolution, const ProgressCallback& onProgress) const override
	{
		return ScalarFieldSampler::Sample(*field, resolution, onProgress);
	}

private:
	std::shared_ptr<IScalarField> field;
};
