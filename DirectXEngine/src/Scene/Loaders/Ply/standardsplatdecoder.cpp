#include "standardsplatdecoder.h"
#include "plyheaderparser.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Ply
{
	namespace
	{
		// Zeroth order spherical harmonics coefficient used by 3DGS for base colour.
		constexpr float kSphericalHarmonicsC0 = 0.28209479177387814f;
		constexpr float kMinScale = 0.0001f;
		constexpr float kMaxScale = 10.0f;

		enum Slot
		{
			PositionX, PositionY, PositionZ,
			ScaleX, ScaleY, ScaleZ,
			RotationW, RotationX, RotationY, RotationZ,
			DiffuseR, DiffuseG, DiffuseB,
			Opacity,
			Red, Green, Blue, Alpha,
			SlotCount
		};

		const std::array<const char*, SlotCount>& SlotPropertyNames()
		{
			static const std::array<const char*, SlotCount> names =
			{
				"x", "y", "z",
				"scale_0", "scale_1", "scale_2",
				"rot_0", "rot_1", "rot_2", "rot_3",
				"f_dc_0", "f_dc_1", "f_dc_2",
				"opacity",
				"red", "green", "blue", "alpha"
			};
			return names;
		}

		float Clamp01(float value)
		{
			return std::max(0.0f, std::min(1.0f, value));
		}

		float Sigmoid(float value)
		{
			return 1.0f / (1.0f + std::exp(-value));
		}
	}

	bool StandardSplatDecoder::CanDecode(const Header& header) const
	{
		const Element* vertexElement = FindElement(header, "vertex");
		return vertexElement != nullptr
			&& FindPropertyIndex(vertexElement->properties, "x") >= 0
			&& FindPropertyIndex(vertexElement->properties, "y") >= 0
			&& FindPropertyIndex(vertexElement->properties, "z") >= 0;
	}

	bool StandardSplatDecoder::Decode(
		std::istream& input,
		const Header& header,
		GaussianSplatCloud& out,
		std::string* outError) const
	{
		const Element* vertexElement = FindElement(header, "vertex");
		if (vertexElement == nullptr || vertexElement->count == 0)
		{
			if (outError) *outError = "PLY file has no vertices.";
			return false;
		}

		// Ask the reader for the handful of properties we understand; the dozens
		// of higher order SH coefficients are parsed and dropped.
		std::vector<int> requested(SlotCount, -1);
		std::array<bool, SlotCount> present{};
		for (int slot = 0; slot < SlotCount; ++slot)
		{
			requested[static_cast<std::size_t>(slot)] = FindPropertyIndex(vertexElement->properties, SlotPropertyNames()[static_cast<std::size_t>(slot)]);
			present[static_cast<std::size_t>(slot)] = requested[static_cast<std::size_t>(slot)] >= 0;
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

		const bool hasSphericalColor = present[DiffuseR] && present[DiffuseG] && present[DiffuseB];
		const bool hasByteColor = present[Red] && present[Green] && present[Blue];
		const bool hasScale = present[ScaleX] && present[ScaleY] && present[ScaleZ];
		const bool hasRotation = present[RotationW] && present[RotationX] && present[RotationY] && present[RotationZ];

		out.splats.clear();
		out.splats.reserve(data.recordCount);

		for (std::size_t record = 0; record < data.recordCount; ++record)
		{
			auto value = [&](Slot slot) { return static_cast<float>(data.Scalar(record, static_cast<std::size_t>(slot))); };

			GaussianSplat splat;
			splat.center = DirectX::XMFLOAT3(value(PositionX), value(PositionY), value(PositionZ));

			float red = 1.0f;
			float green = 1.0f;
			float blue = 1.0f;
			if (hasSphericalColor)
			{
				red = Clamp01(0.5f + kSphericalHarmonicsC0 * value(DiffuseR));
				green = Clamp01(0.5f + kSphericalHarmonicsC0 * value(DiffuseG));
				blue = Clamp01(0.5f + kSphericalHarmonicsC0 * value(DiffuseB));
			}
			else if (hasByteColor)
			{
				red = Clamp01(value(Red) / 255.0f);
				green = Clamp01(value(Green) / 255.0f);
				blue = Clamp01(value(Blue) / 255.0f);
			}

			float opacity = 1.0f;
			if (present[Opacity])
				opacity = Sigmoid(value(Opacity)); // stored as a logit
			else if (present[Alpha])
				opacity = Clamp01(value(Alpha) / 255.0f);

			splat.color = DirectX::XMFLOAT4(red, green, blue, opacity);

			// 3DGS stores log-scales, so exp() brings them back to world units.
			float scaleX = 0.01f;
			float scaleY = 0.01f;
			float scaleZ = 0.01f;
			if (hasScale)
			{
				scaleX = std::exp(value(ScaleX));
				scaleY = std::exp(value(ScaleY));
				scaleZ = std::exp(value(ScaleZ));
			}

			splat.scale = DirectX::XMFLOAT3(
				std::clamp(scaleX, kMinScale, kMaxScale),
				std::clamp(scaleY, kMinScale, kMaxScale),
				std::clamp(scaleZ, kMinScale, kMaxScale));

			if (hasRotation)
			{
				float rotationW = value(RotationW);
				float rotationX = value(RotationX);
				float rotationY = value(RotationY);
				float rotationZ = value(RotationZ);
				const float length = std::sqrt(
					rotationW * rotationW + rotationX * rotationX + rotationY * rotationY + rotationZ * rotationZ);

				if (length > 1e-8f)
				{
					splat.rotation = DirectX::XMFLOAT4(rotationX / length, rotationY / length, rotationZ / length, rotationW / length);
				}
			}

			out.splats.push_back(splat);
		}

		out.ComputeBounds();
		return true;
	}
}
