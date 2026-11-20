#pragma once
#include "isplatdecoder.h"

namespace Ply
{
	// Uncompressed INRIA / "3D Gaussian Splatting" layout: one `vertex` element
	// whose properties carry raw floats (x/y/z, scale_*, rot_*, f_dc_*, opacity).
	// Also handles plain coloured point clouds (red/green/blue) as a fallback.
	class StandardSplatDecoder final : public ISplatDecoder
	{
	public:
		const char* GetName() const override { return "3DGS PLY"; }
		bool CanDecode(const Header& header) const override;
		bool Decode(std::istream& input, const Header& header, GaussianSplatCloud& out, std::string* outError) const override;
	};
}
