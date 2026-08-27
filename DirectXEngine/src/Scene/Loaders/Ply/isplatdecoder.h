#pragma once
#include "gaussiansplat.h"
#include "plyelementreader.h"
#include "plytypes.h"

#include <istream>
#include <string>

namespace Ply
{
	// Strategy for turning one particular splat PLY dialect into GaussianSplats.
	// Supporting a new dialect means adding an implementation and registering it
	// in GaussianSplatPlyLoader - no existing decoder has to change.
	class ISplatDecoder
	{
	public:
		virtual ~ISplatDecoder() = default;

		// Format name shown in log messages.
		virtual const char* GetName() const = 0;
		// True when this decoder recognises the element/property layout.
		virtual bool CanDecode(const Header& header) const = 0;
		// Consume the data section of `input` and fill `out`.
		virtual bool Decode(
			std::istream& input,
			const Header& header,
			GaussianSplatCloud& out,
			std::string* outError) const = 0;
	};
}
