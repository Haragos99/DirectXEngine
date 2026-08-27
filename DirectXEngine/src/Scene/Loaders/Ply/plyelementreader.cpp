#include "plyelementreader.h"

#include <limits>

namespace Ply
{
	namespace
	{
		std::size_t ScalarTypeSize(ScalarType type)
		{
			switch (type)
			{
			case ScalarType::Int8:
			case ScalarType::UInt8:
				return 1;
			case ScalarType::Int16:
			case ScalarType::UInt16:
				return 2;
			case ScalarType::Int32:
			case ScalarType::UInt32:
			case ScalarType::Float32:
				return 4;
			case ScalarType::Float64:
				return 8;
			default:
				return 0;
			}
		}

		template <typename ValueType>
		ValueType ReadRaw(std::istream& input)
		{
			ValueType value{};
			input.read(reinterpret_cast<char*>(&value), sizeof(ValueType));
			return value;
		}

		// binary_little_endian only, which matches the little endian x64 target.
		double ReadBinaryScalar(std::istream& input, ScalarType type)
		{
			switch (type)
			{
			case ScalarType::Int8:    return static_cast<double>(ReadRaw<std::int8_t>(input));
			case ScalarType::UInt8:   return static_cast<double>(ReadRaw<std::uint8_t>(input));
			case ScalarType::Int16:   return static_cast<double>(ReadRaw<std::int16_t>(input));
			case ScalarType::UInt16:  return static_cast<double>(ReadRaw<std::uint16_t>(input));
			case ScalarType::Int32:   return static_cast<double>(ReadRaw<std::int32_t>(input));
			case ScalarType::UInt32:  return static_cast<double>(ReadRaw<std::uint32_t>(input));
			case ScalarType::Float32: return static_cast<double>(ReadRaw<float>(input));
			case ScalarType::Float64: return ReadRaw<double>(input);
			default:                  return 0.0;
			}
		}

		double ReadScalar(std::istream& input, Format format, ScalarType type)
		{
			if (format == Format::Ascii)
			{
				double value = 0.0;
				input >> value;
				return value;
			}
			return ReadBinaryScalar(input, type);
		}

		std::uint32_t ToListIndex(double value)
		{
			return value > 0.0 ? static_cast<std::uint32_t>(value) : 0u;
		}
	}

	bool ReadElement(
		std::istream& input,
		Format format,
		const Element& element,
		const std::vector<int>& requestedScalars,
		const std::vector<int>& requestedLists,
		ElementData& out,
		std::string* outError)
	{
		out.recordCount = element.count;
		out.scalarSlots = requestedScalars.size();
		out.listSlots = requestedLists.size();

		// Guard against a corrupt header claiming an allocation we cannot serve.
		const std::size_t maxRecords = std::numeric_limits<std::size_t>::max() / (out.scalarSlots + out.listSlots + 1);
		if (element.count > maxRecords)
		{
			if (outError) *outError = "PLY element declares an implausible record count.";
			return false;
		}

		out.scalars.assign(element.count * out.scalarSlots, 0.0);
		out.lists.assign(element.count * out.listSlots, {});

		// property index -> destination slot (-1 when the property is not wanted)
		std::vector<int> scalarSlotOf(element.properties.size(), -1);
		std::vector<int> listSlotOf(element.properties.size(), -1);
		for (std::size_t slot = 0; slot < requestedScalars.size(); ++slot)
		{
			const int propertyIndex = requestedScalars[slot];
			if (propertyIndex >= 0 && propertyIndex < static_cast<int>(scalarSlotOf.size()))
				scalarSlotOf[static_cast<std::size_t>(propertyIndex)] = static_cast<int>(slot);
		}
		for (std::size_t slot = 0; slot < requestedLists.size(); ++slot)
		{
			const int propertyIndex = requestedLists[slot];
			if (propertyIndex >= 0 && propertyIndex < static_cast<int>(listSlotOf.size()))
				listSlotOf[static_cast<std::size_t>(propertyIndex)] = static_cast<int>(slot);
		}

		for (std::size_t record = 0; record < element.count; ++record)
		{
			for (std::size_t propertyIndex = 0; propertyIndex < element.properties.size(); ++propertyIndex)
			{
				const Property& property = element.properties[propertyIndex];
				if (ScalarTypeSize(property.type) == 0)
				{
					if (outError) *outError = "PLY element contains a property with an invalid type.";
					return false;
				}

				if (property.isList)
				{
					const std::size_t entryCount = static_cast<std::size_t>(ReadScalar(input, format, property.countType));
					const int slot = listSlotOf[propertyIndex];
					std::vector<std::uint32_t>* target = slot >= 0
						? &out.lists[record * out.listSlots + static_cast<std::size_t>(slot)]
						: nullptr;

					if (target)
						target->reserve(entryCount);

					for (std::size_t entry = 0; entry < entryCount; ++entry)
					{
						const double value = ReadScalar(input, format, property.type);
						if (target)
							target->push_back(ToListIndex(value));
					}
				}
				else
				{
					const double value = ReadScalar(input, format, property.type);
					const int slot = scalarSlotOf[propertyIndex];
					if (slot >= 0)
						out.scalars[record * out.scalarSlots + static_cast<std::size_t>(slot)] = value;
				}
			}

			if (!input)
			{
				if (outError) *outError = "Unexpected end of PLY element data.";
				return false;
			}
		}

		return true;
	}

	bool SkipElement(std::istream& input, Format format, const Element& element, std::string* outError)
	{
		ElementData discarded;
		return ReadElement(input, format, element, {}, {}, discarded, outError);
	}
}
