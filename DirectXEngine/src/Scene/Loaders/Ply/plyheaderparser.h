#pragma once
#include "plytypes.h"

#include <istream>
#include <string>

namespace Ply
{
	// Read the textual header and leave `input` positioned at the first data byte.
	bool ParseHeader(std::istream& input, Header& header, std::string* outError);

	// Element with the given name, or nullptr when the file does not contain it.
	const Element* FindElement(const Header& header, const std::string& elementName);

	// Index of a property inside an element, or -1 when it is absent.
	int FindPropertyIndex(const std::vector<Property>& properties, const std::string& propertyName);
}
