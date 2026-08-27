#include "plyheaderparser.h"

#include <sstream>

namespace Ply
{
	namespace
	{
		ScalarType ParseScalarType(const std::string& typeName)
		{
			if (typeName == "char" || typeName == "int8")
				return ScalarType::Int8;
			if (typeName == "uchar" || typeName == "uint8")
				return ScalarType::UInt8;
			if (typeName == "short" || typeName == "int16")
				return ScalarType::Int16;
			if (typeName == "ushort" || typeName == "uint16")
				return ScalarType::UInt16;
			if (typeName == "int" || typeName == "int32")
				return ScalarType::Int32;
			if (typeName == "uint" || typeName == "uint32")
				return ScalarType::UInt32;
			if (typeName == "float" || typeName == "float32")
				return ScalarType::Float32;
			if (typeName == "double" || typeName == "float64")
				return ScalarType::Float64;

			return ScalarType::Invalid;
		}
	}

	bool ParseHeader(std::istream& input, Header& header, std::string* outError)
	{
		std::string line;
		if (!std::getline(input, line))
		{
			if (outError) *outError = "PLY file is empty.";
			return false;
		}

		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line != "ply")
		{
			if (outError) *outError = "File is not a valid PLY file. Missing 'ply' magic line.";
			return false;
		}

		int currentElementIndex = -1;

		while (std::getline(input, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (line == "end_header")
				return true;

			if (line.empty() || line.rfind("comment", 0) == 0)
				continue;

			std::istringstream lineStream(line);
			std::string token;
			lineStream >> token;

			if (token == "format")
			{
				std::string formatName;
				lineStream >> formatName;

				if (formatName == "ascii")
				{
					header.format = Format::Ascii;
				}
				else if (formatName == "binary_little_endian")
				{
					header.format = Format::BinaryLittleEndian;
				}
				else
				{
					if (outError) *outError = "Unsupported PLY format: " + formatName;
					return false;
				}
			}
			else if (token == "element")
			{
				Element element;
				lineStream >> element.name >> element.count;
				header.elements.push_back(element);
				currentElementIndex = static_cast<int>(header.elements.size()) - 1;
			}
			else if (token == "property" && currentElementIndex >= 0)
			{
				std::string typeName;
				lineStream >> typeName;

				Property property;
				if (typeName == "list")
				{
					std::string countTypeName;
					std::string valueTypeName;
					lineStream >> countTypeName >> valueTypeName >> property.name;
					property.isList = true;
					property.countType = ParseScalarType(countTypeName);
					property.type = ParseScalarType(valueTypeName);

					if (property.countType == ScalarType::Invalid || property.type == ScalarType::Invalid)
					{
						if (outError) *outError = "Unsupported PLY list property type in: " + line;
						return false;
					}
				}
				else
				{
					lineStream >> property.name;
					property.type = ParseScalarType(typeName);

					if (property.type == ScalarType::Invalid)
					{
						if (outError) *outError = "Unsupported PLY property type: " + typeName;
						return false;
					}
				}

				header.elements[static_cast<std::size_t>(currentElementIndex)].properties.push_back(property);
			}
		}

		if (outError) *outError = "PLY header ended unexpectedly.";
		return false;
	}

	const Element* FindElement(const Header& header, const std::string& elementName)
	{
		for (const Element& element : header.elements)
		{
			if (element.name == elementName)
				return &element;
		}
		return nullptr;
	}

	int FindPropertyIndex(const std::vector<Property>& properties, const std::string& propertyName)
	{
		for (std::size_t propertyIndex = 0; propertyIndex < properties.size(); ++propertyIndex)
		{
			if (properties[propertyIndex].name == propertyName)
				return static_cast<int>(propertyIndex);
		}
		return -1;
	}
}
