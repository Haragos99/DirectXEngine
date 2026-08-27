#include "supersplatdecoder.h"
#include "plyheaderparser.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Ply
{
	namespace
	{
		// SuperSplat groups splats into fixed size chunks that share a quantisation range.
		constexpr std::size_t kChunkSize = 256;
		constexpr float kMinScale = 0.0001f;
		constexpr float kMaxScale = 10.0f;

		enum ChunkSlot
		{
			MinX, MinY, MinZ, MaxX, MaxY, MaxZ,
			MinScaleX, MinScaleY, MinScaleZ, MaxScaleX, MaxScaleY, MaxScaleZ,
			MinR, MinG, MinB, MaxR, MaxG, MaxB,
			ChunkSlotCount
		};

		enum VertexSlot
		{
			PackedPosition, PackedRotation, PackedScale, PackedColor,
			VertexSlotCount
		};

		const std::array<const char*, ChunkSlotCount>& ChunkPropertyNames()
		{
			static const std::array<const char*, ChunkSlotCount> names =
			{
				"min_x", "min_y", "min_z", "max_x", "max_y", "max_z",
				"min_scale_x", "min_scale_y", "min_scale_z", "max_scale_x", "max_scale_y", "max_scale_z",
				"min_r", "min_g", "min_b", "max_r", "max_g", "max_b"
			};
			return names;
		}

		const std::array<const char*, VertexSlotCount>& VertexPropertyNames()
		{
			static const std::array<const char*, VertexSlotCount> names =
			{
				"packed_position", "packed_rotation", "packed_scale", "packed_color"
			};
			return names;
		}

		float Clamp01(float value)
		{
			return std::max(0.0f, std::min(1.0f, value));
		}

		float Lerp(float minimum, float maximum, float normalized)
		{
			return minimum + (maximum - minimum) * normalized;
		}

		// packed_position and packed_scale share an 11/10/11 bit layout.
		void UnpackTriplet(std::uint32_t packed, float& x, float& y, float& z)
		{
			constexpr std::uint32_t kXMask = 0x7FFu; // 11 bits
			constexpr std::uint32_t kYMask = 0x3FFu; // 10 bits
			constexpr std::uint32_t kZMask = 0x7FFu; // 11 bits

			x = static_cast<float>((packed >> 21) & kXMask) / 2047.0f;
			y = static_cast<float>((packed >> 11) & kYMask) / 1023.0f;
			z = static_cast<float>(packed & kZMask) / 2047.0f;
		}

		// "Smallest three" quaternion encoding: the largest component is dropped
		// and rebuilt from the unit length constraint.
		DirectX::XMFLOAT4 UnpackRotation(std::uint32_t packed)
		{
			constexpr std::uint32_t kComponentMask = 0x3FFu; // 10 bits
			constexpr float kComponentMax = 1023.0f;
			constexpr float kSqrtTwo = 1.4142135623730951f;

			const float a = (static_cast<float>((packed >> 20) & kComponentMask) / kComponentMax - 0.5f) * kSqrtTwo;
			const float b = (static_cast<float>((packed >> 10) & kComponentMask) / kComponentMax - 0.5f) * kSqrtTwo;
			const float c = (static_cast<float>(packed & kComponentMask) / kComponentMax - 0.5f) * kSqrtTwo;

			const float missing = std::sqrt(std::max(0.0f, 1.0f - (a * a + b * b + c * c)));

			// XMFLOAT4 is (x, y, z, w) while the encoding orders components as (w, x, y, z).
			switch (packed >> 30)
			{
			case 0:  return DirectX::XMFLOAT4(a, b, c, missing);
			case 1:  return DirectX::XMFLOAT4(missing, b, c, a);
			case 2:  return DirectX::XMFLOAT4(b, missing, c, a);
			default: return DirectX::XMFLOAT4(b, c, missing, a);
			}
		}
	}

	bool SuperSplatDecoder::CanDecode(const Header& header) const
	{
		const Element* chunkElement = FindElement(header, "chunk");
		const Element* vertexElement = FindElement(header, "vertex");

		return header.format == Format::BinaryLittleEndian
			&& chunkElement != nullptr
			&& vertexElement != nullptr
			&& FindPropertyIndex(vertexElement->properties, "packed_position") >= 0
			&& FindPropertyIndex(vertexElement->properties, "packed_scale") >= 0
			&& FindPropertyIndex(vertexElement->properties, "packed_color") >= 0;
	}

	bool SuperSplatDecoder::Decode(
		std::istream& input,
		const Header& header,
		GaussianSplatCloud& out,
		std::string* outError) const
	{
		const Element* chunkElement = FindElement(header, "chunk");
		const Element* vertexElement = FindElement(header, "vertex");
		if (chunkElement == nullptr || vertexElement == nullptr)
		{
			if (outError) *outError = "Compressed PLY is missing the chunk or vertex element.";
			return false;
		}

		std::vector<int> chunkRequested(ChunkSlotCount, -1);
		std::array<bool, ChunkSlotCount> chunkPresent{};
		for (int slot = 0; slot < ChunkSlotCount; ++slot)
		{
			chunkRequested[static_cast<std::size_t>(slot)] = FindPropertyIndex(chunkElement->properties, ChunkPropertyNames()[static_cast<std::size_t>(slot)]);
			chunkPresent[static_cast<std::size_t>(slot)] = chunkRequested[static_cast<std::size_t>(slot)] >= 0;
		}

		std::vector<int> vertexRequested(VertexSlotCount, -1);
		std::array<bool, VertexSlotCount> vertexPresent{};
		for (int slot = 0; slot < VertexSlotCount; ++slot)
		{
			vertexRequested[static_cast<std::size_t>(slot)] = FindPropertyIndex(vertexElement->properties, VertexPropertyNames()[static_cast<std::size_t>(slot)]);
			vertexPresent[static_cast<std::size_t>(slot)] = vertexRequested[static_cast<std::size_t>(slot)] >= 0;
		}

		ElementData chunkData;
		ElementData vertexData;
		for (const Element& element : header.elements)
		{
			if (&element == chunkElement)
			{
				if (!ReadElement(input, header.format, element, chunkRequested, {}, chunkData, outError))
					return false;
			}
			else if (&element == vertexElement)
			{
				if (!ReadElement(input, header.format, element, vertexRequested, {}, vertexData, outError))
					return false;
				break;
			}
			else if (!SkipElement(input, header.format, element, outError))
			{
				return false;
			}
		}

		out.splats.clear();
		out.splats.reserve(vertexData.recordCount);

		for (std::size_t vertexIndex = 0; vertexIndex < vertexData.recordCount; ++vertexIndex)
		{
			const std::size_t chunkIndex = vertexIndex / kChunkSize;
			if (chunkIndex >= chunkData.recordCount)
			{
				if (outError) *outError = "Compressed PLY vertex references a missing chunk.";
				return false;
			}

			auto range = [&](ChunkSlot slot, float fallback)
			{
				return chunkPresent[static_cast<std::size_t>(slot)]
					? static_cast<float>(chunkData.Scalar(chunkIndex, static_cast<std::size_t>(slot)))
					: fallback;
			};
			auto packed = [&](VertexSlot slot)
			{
				return vertexPresent[static_cast<std::size_t>(slot)]
					? static_cast<std::uint32_t>(vertexData.Scalar(vertexIndex, static_cast<std::size_t>(slot)))
					: 0u;
			};

			float normalizedX = 0.0f;
			float normalizedY = 0.0f;
			float normalizedZ = 0.0f;

			GaussianSplat splat;

			UnpackTriplet(packed(PackedPosition), normalizedX, normalizedY, normalizedZ);
			splat.center = DirectX::XMFLOAT3(
				Lerp(range(MinX, 0.0f), range(MaxX, 0.0f), normalizedX),
				Lerp(range(MinY, 0.0f), range(MaxY, 0.0f), normalizedY),
				Lerp(range(MinZ, 0.0f), range(MaxZ, 0.0f), normalizedZ));

			UnpackTriplet(packed(PackedScale), normalizedX, normalizedY, normalizedZ);
			splat.scale = DirectX::XMFLOAT3(
				std::clamp(std::exp(Lerp(range(MinScaleX, 0.0f), range(MaxScaleX, 0.0f), normalizedX)), kMinScale, kMaxScale),
				std::clamp(std::exp(Lerp(range(MinScaleY, 0.0f), range(MaxScaleY, 0.0f), normalizedY)), kMinScale, kMaxScale),
				std::clamp(std::exp(Lerp(range(MinScaleZ, 0.0f), range(MaxScaleZ, 0.0f), normalizedZ)), kMinScale, kMaxScale));

			const std::uint32_t colorValue = packed(PackedColor);
			splat.color = DirectX::XMFLOAT4(
				Clamp01(Lerp(range(MinR, 0.0f), range(MaxR, 1.0f), static_cast<float>((colorValue >> 24) & 0xFFu) / 255.0f)),
				Clamp01(Lerp(range(MinG, 0.0f), range(MaxG, 1.0f), static_cast<float>((colorValue >> 16) & 0xFFu) / 255.0f)),
				Clamp01(Lerp(range(MinB, 0.0f), range(MaxB, 1.0f), static_cast<float>((colorValue >> 8) & 0xFFu) / 255.0f)),
				static_cast<float>(colorValue & 0xFFu) / 255.0f);

			if (vertexPresent[PackedRotation])
				splat.rotation = UnpackRotation(packed(PackedRotation));

			out.splats.push_back(splat);
		}

		out.ComputeBounds();
		return true;
	}
}
