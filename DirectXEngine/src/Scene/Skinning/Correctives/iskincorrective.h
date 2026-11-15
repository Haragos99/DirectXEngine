#pragma once
#include "../skinbindcontext.h"
#include "../skinweights.h"

#include <d3d11.h>

class ISkinCorrective
{
public:
	virtual ~ISkinCorrective() = default;

	virtual const char* GetName() const = 0;

	virtual bool Prepare(const SkinBindContext& context, const SkinWeightTable& weights) = 0;

	virtual void Apply(ID3D11UnorderedAccessView* deformedVertices) = 0;
};
