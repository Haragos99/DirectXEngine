#pragma once
#include "isplatdecoder.h"

namespace Ply
{
	// PlayCanvas SuperSplat / "compressed" layout: a `chunk` element holding the
	// per-256-splat min/max ranges plus a `vertex` element of bit packed uint32s.
	class SuperSplatDecoder final : public ISplatDecoder
	{
	public:
		const char* GetName() const override { return "SuperSplat compressed PLY"; }
		bool CanDecode(const Header& header) const override;
		bool Decode(std::istream& input, const Header& header, GaussianSplatCloud& out, std::string* outError) const override;
	};
}
