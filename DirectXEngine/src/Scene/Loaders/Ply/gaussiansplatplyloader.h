#pragma once
#include "gaussiansplat.h"
#include "isplatdecoder.h"

#include <memory>
#include <string>
#include <vector>

namespace Ply
{
	// Facade over the splat decoders: opens the file, parses the header and hands
	// the data section to the first decoder that recognises the layout.
	class GaussianSplatPlyLoader
	{
	public:
		GaussianSplatPlyLoader();

		// True when `path` is a PLY that carries Gaussian splat attributes.
		// Cheap: only the header is read.
		static bool IsSplatFile(const std::string& path);

		bool Load(const std::string& path, GaussianSplatCloud& out, std::string* outError = nullptr) const;

		void Register(std::shared_ptr<ISplatDecoder> decoder);

	private:
		std::vector<std::shared_ptr<ISplatDecoder>> decoders;
	};
}
