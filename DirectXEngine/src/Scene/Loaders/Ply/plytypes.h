#pragma once
#include <cstddef>
#include <string>
#include <vector>

// Low level description of a PLY file header. These types are pure data: they
// know the file layout but nothing about what the data means.
namespace Ply
{
	enum class Format
	{
		Unknown,
		Ascii,
		BinaryLittleEndian
	};

	enum class ScalarType
	{
		Invalid,
		Int8,
		UInt8,
		Int16,
		UInt16,
		Int32,
		UInt32,
		Float32,
		Float64
	};

	struct Property
	{
		std::string name;
		ScalarType type = ScalarType::Invalid;
		// A "property list" (used by face indices) instead of a single scalar.
		bool isList = false;
		ScalarType countType = ScalarType::Invalid;
	};

	struct Element
	{
		std::string name;
		std::size_t count = 0;
		std::vector<Property> properties;
	};

	struct Header
	{
		Format format = Format::Unknown;
		std::vector<Element> elements;
	};
}
