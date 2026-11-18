#pragma once
#include "plytypes.h"

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace Ply
{
	// Flat, cache friendly storage for the properties a decoder actually asked for.
	// Splat files routinely contain millions of records with 60+ properties, so
	// everything that was not requested is parsed and thrown away instead of stored.
	// Values are kept as double so 32 bit packed integers survive without loss.
	struct ElementData
	{
		std::size_t recordCount = 0;
		std::size_t scalarSlots = 0;
		std::size_t listSlots = 0;
		std::vector<double> scalars;                    // recordCount * scalarSlots
		std::vector<std::vector<std::uint32_t>> lists;  // recordCount * listSlots

		double Scalar(std::size_t record, std::size_t slot) const
		{
			return scalars[record * scalarSlots + slot];
		}

		const std::vector<std::uint32_t>& List(std::size_t record, std::size_t slot) const
		{
			return lists[record * listSlots + slot];
		}
	};

	// Reads `element`, keeping only the properties named by `requestedScalars` /
	// `requestedLists` (indices into element.properties, order defines the slots).
	bool ReadElement(
		std::istream& input,
		Format format,
		const Element& element,
		const std::vector<int>& requestedScalars,
		const std::vector<int>& requestedLists,
		ElementData& out,
		std::string* outError);

	// Advances past an element's data without keeping any of it.
	bool SkipElement(std::istream& input, Format format, const Element& element, std::string* outError);
}
