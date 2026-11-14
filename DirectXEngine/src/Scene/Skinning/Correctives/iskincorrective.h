#pragma once
#include "skinbindcontext.h"
#include "skinweights.h"

#include <d3d11.h>

// Optional third stage of skinning, run on the deformer's output every frame:
// Delta Mush and Improved Delta Mush smooth the deformed mesh and re-add the
// rest pose detail, Implicit Skinning projects it onto per-bone fields.
// Neither needs anything from the stages before it beyond what is passed here.
//
// No implementation yet; SkinningController does not run correctives.
class ISkinCorrective
{
public:
	virtual ~ISkinCorrective() = default;

	virtual const char* GetName() const = 0;

	// Builds everything that stays fixed while the mesh stays bound.
	virtual bool Prepare(const SkinBindContext& context, const SkinWeightTable& weights) = 0;

	// Rewrites the deformed vertices in place (raw UAV over the vertex buffer).
	virtual void Apply(ID3D11UnorderedAccessView* deformedVertices) = 0;
};
