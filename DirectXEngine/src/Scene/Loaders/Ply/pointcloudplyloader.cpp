#include "pointcloudplyloader.h"
#include "plyelementreader.h"
#include "plyheaderparser.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>

namespace Ply
{
	namespace
	{
		constexpr float kSphericalHarmonicsC0 = 0.28209479177387814f;

		enum Slot
		{
			PositionX, PositionY, PositionZ,
			Red, Green, Blue, Alpha,
			DiffuseR, DiffuseG, DiffuseB, Opacity,
			SlotCount
		};

		const std::array<const char*, SlotCount>& SlotPropertyNames()
		{
			static const std::array<const char*, SlotCount> names =
			{
				"x", "y", "z",
				"red", "green", "blue", "alpha",
				"f_dc_0", "f_dc_1", "f_dc_2", "opacity"
			};
			return names;
		}

		float Clamp01(float value)
		{
			return std::max(0.0f, std::min(1.0f, value));
		}

		// Integer channels are 0..255, float channels are already normalised.
		float ChannelToUnit(float value, ScalarType type)
		{
			return type == ScalarType::Float32 || type == ScalarType::Float64
				? Clamp01(value)
				: Clamp01(value / 255.0f);
		}

		bool HasPositions(const Element& vertexElement)
		{
			return FindPropertyIndex(vertexElement.properties, "x") >= 0
				&& FindPropertyIndex(vertexElement.properties, "y") >= 0
				&& FindPropertyIndex(vertexElement.properties, "z") >= 0;
		}
	}

	bool PointCloudPlyLoader::IsPointCloudFile(const std::string& path)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input)
			return false;

		Header header;
		if (!ParseHeader(input, header, nullptr))
			return false;

		const Element* vertexElement = FindElement(header, "vertex");
		if (vertexElement == nullptr || vertexElement->count == 0 || !HasPositions(*vertexElement))
			return false;

		// A Gaussian capture belongs to the splat loader, and anything with
		// faces is a mesh.
		if (FindPropertyIndex(vertexElement->properties, "scale_0") >= 0
			|| FindPropertyIndex(vertexElement->properties, "packed_position") >= 0)
			return false;

		const Element* faceElement = FindElement(header, "face");
		return faceElement == nullptr || faceElement->count == 0;
	}

	bool PointCloudPlyLoader::Load(const std::string& path, PointCloud& out, std::string* outError) const
	{
		out.points.clear();

		std::ifstream input(path, std::ios::binary);
		if (!input)
		{
			if (outError) *outError = "Could not open PLY file: " + path;
			return false;
		}

		Header header;
		if (!ParseHeader(input, header, outError))
			return false;

		const Element* vertexElement = FindElement(header, "vertex");
		if (vertexElement == nullptr || vertexElement->count == 0)
		{
			if (outError) *outError = "PLY file has no points.";
			return false;
		}

		std::vector<int> requested(SlotCount, -1);
		std::array<bool, SlotCount> present{};
		std::array<ScalarType, SlotCount> types{};
		for (int slot = 0; slot < SlotCount; ++slot)
		{
			const int propertyIndex = FindPropertyIndex(vertexElement->properties, SlotPropertyNames()[static_cast<std::size_t>(slot)]);
			requested[static_cast<std::size_t>(slot)] = propertyIndex;
			present[static_cast<std::size_t>(slot)] = propertyIndex >= 0;
			types[static_cast<std::size_t>(slot)] = propertyIndex >= 0
				? vertexElement->properties[static_cast<std::size_t>(propertyIndex)].type
				: ScalarType::Invalid;
		}

		if (!present[PositionX] || !present[PositionY] || !present[PositionZ])
		{
			if (outError) *outError = "PLY vertex element has no x/y/z properties.";
			return false;
		}

		for (const Element& element : header.elements)
		{
			if (&element == vertexElement)
				break;
			if (!SkipElement(input, header.format, element, outError))
				return false;
		}

		ElementData data;
		if (!ReadElement(input, header.format, *vertexElement, requested, {}, data, outError))
			return false;

		const bool hasByteColor = present[Red] && present[Green] && present[Blue];
		const bool hasSphericalColor = present[DiffuseR] && present[DiffuseG] && present[DiffuseB];

		out.points.reserve(data.recordCount);
		for (std::size_t record = 0; record < data.recordCount; ++record)
		{
			auto value = [&](Slot slot) { return static_cast<float>(data.Scalar(record, static_cast<std::size_t>(slot))); };

			PointCloudPoint point;
			point.position = DirectX::XMFLOAT3(value(PositionX), value(PositionY), value(PositionZ));

			if (hasByteColor)
			{
				point.color = DirectX::XMFLOAT4(
					ChannelToUnit(value(Red), types[Red]),
					ChannelToUnit(value(Green), types[Green]),
					ChannelToUnit(value(Blue), types[Blue]),
					1.0f);
			}
			else if (hasSphericalColor)
			{
				point.color = DirectX::XMFLOAT4(
					Clamp01(0.5f + kSphericalHarmonicsC0 * value(DiffuseR)),
					Clamp01(0.5f + kSphericalHarmonicsC0 * value(DiffuseG)),
					Clamp01(0.5f + kSphericalHarmonicsC0 * value(DiffuseB)),
					1.0f);
			}

			if (present[Alpha])
				point.color.w = ChannelToUnit(value(Alpha), types[Alpha]);
			else if (present[Opacity])
				point.color.w = Clamp01(1.0f / (1.0f + std::exp(-value(Opacity))));

			out.points.push_back(point);
		}

		out.ComputeBounds();
		return !out.points.empty();
	}
}
