#include "gaussiansplatplyloader.h"
#include "plyheaderparser.h"
#include "standardsplatdecoder.h"
#include "supersplatdecoder.h"

#include <fstream>

namespace Ply
{
	GaussianSplatPlyLoader::GaussianSplatPlyLoader()
	{
		// Most specific dialect first: the compressed layout is a strict subset
		// check, while the standard decoder accepts almost any vertex element.
		Register(std::make_shared<SuperSplatDecoder>());
		Register(std::make_shared<StandardSplatDecoder>());
	}

	void GaussianSplatPlyLoader::Register(std::shared_ptr<ISplatDecoder> decoder)
	{
		if (decoder)
			decoders.push_back(std::move(decoder));
	}

	bool GaussianSplatPlyLoader::IsSplatFile(const std::string& path)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input)
			return false;

		Header header;
		if (!ParseHeader(input, header, nullptr))
			return false;

		const Element* vertexElement = FindElement(header, "vertex");
		if (vertexElement == nullptr)
			return false;

		// Attributes that only exist in Gaussian splat exports.
		return FindPropertyIndex(vertexElement->properties, "scale_0") >= 0
			|| FindPropertyIndex(vertexElement->properties, "packed_position") >= 0;
	}

	bool GaussianSplatPlyLoader::Load(const std::string& path, GaussianSplatCloud& out, std::string* outError) const
	{
		out.splats.clear();

		std::ifstream input(path, std::ios::binary);
		if (!input)
		{
			if (outError) *outError = "Could not open PLY file: " + path;
			return false;
		}

		Header header;
		if (!ParseHeader(input, header, outError))
			return false;

		if (header.format == Format::Unknown)
		{
			if (outError) *outError = "PLY format line is missing or unsupported.";
			return false;
		}

		for (const std::shared_ptr<ISplatDecoder>& decoder : decoders)
		{
			if (!decoder->CanDecode(header))
				continue;

			if (decoder->Decode(input, header, out, outError))
				return true;

			out.splats.clear();
			return false;
		}

		if (outError) *outError = "PLY file does not match a supported Gaussian splat layout.";
		return false;
	}
}
