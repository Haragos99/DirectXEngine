#include "plymodelloader.h"
#include "Ply/plyelementreader.h"
#include "Ply/plyheaderparser.h"

#include <fstream>
#include <iostream>

using namespace DirectX;

namespace
{
	enum VertexSlot
	{
		PositionX, PositionY, PositionZ,
		NormalX, NormalY, NormalZ,
		TexU, TexV,
		VertexSlotCount
	};

	// The same attribute goes by several names depending on the exporter.
	int FindAny(const std::vector<Ply::Property>& properties, std::initializer_list<const char*> names)
	{
		for (const char* name : names)
		{
			const int index = Ply::FindPropertyIndex(properties, name);
			if (index >= 0)
				return index;
		}
		return -1;
	}

	void ComputeSmoothNormals(MeshData& out)
	{
		out.normals.assign(out.positions.size(), XMFLOAT3(0.0f, 0.0f, 0.0f));

		for (const FaceHandle& face : out.faces)
		{
			if (face.size() < 3)
				continue;

			const XMVECTOR a = XMLoadFloat3(&out.positions[face.vertexIndices[0]]);
			const XMVECTOR b = XMLoadFloat3(&out.positions[face.vertexIndices[1]]);
			const XMVECTOR c = XMLoadFloat3(&out.positions[face.vertexIndices[2]]);
			const XMVECTOR faceNormal = XMVector3Cross(XMVectorSubtract(b, a), XMVectorSubtract(c, a));

			for (UINT vertexIndex : face.vertexIndices)
			{
				const XMVECTOR accumulated = XMLoadFloat3(&out.normals[vertexIndex]);
				XMStoreFloat3(&out.normals[vertexIndex], XMVectorAdd(accumulated, faceNormal));
			}
		}

		for (XMFLOAT3& normal : out.normals)
			XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));
	}
}

bool PlyModelLoader::Load(const std::string& path, MeshData& out) const
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		std::cerr << "Failed to open PLY: " << path << '\n';
		return false;
	}

	Ply::Header header;
	std::string error;
	if (!Ply::ParseHeader(input, header, &error))
	{
		std::cerr << error << '\n';
		return false;
	}

	const Ply::Element* vertexElement = Ply::FindElement(header, "vertex");
	if (vertexElement == nullptr || vertexElement->count == 0)
	{
		std::cerr << "PLY has no vertex element: " << path << '\n';
		return false;
	}

	out.Clear();

	std::vector<int> vertexRequested(VertexSlotCount, -1);
	vertexRequested[PositionX] = FindAny(vertexElement->properties, { "x" });
	vertexRequested[PositionY] = FindAny(vertexElement->properties, { "y" });
	vertexRequested[PositionZ] = FindAny(vertexElement->properties, { "z" });
	vertexRequested[NormalX] = FindAny(vertexElement->properties, { "nx" });
	vertexRequested[NormalY] = FindAny(vertexElement->properties, { "ny" });
	vertexRequested[NormalZ] = FindAny(vertexElement->properties, { "nz" });
	vertexRequested[TexU] = FindAny(vertexElement->properties, { "s", "u", "texture_u" });
	vertexRequested[TexV] = FindAny(vertexElement->properties, { "t", "v", "texture_v" });

	const bool hasNormals = vertexRequested[NormalX] >= 0 && vertexRequested[NormalY] >= 0 && vertexRequested[NormalZ] >= 0;
	const bool hasTexcoords = vertexRequested[TexU] >= 0 && vertexRequested[TexV] >= 0;

	const Ply::Element* faceElement = Ply::FindElement(header, "face");

	// Elements are stored in header order, so walk them and pick up what we need.
	Ply::ElementData vertexData;
	Ply::ElementData faceData;
	for (const Ply::Element& element : header.elements)
	{
		bool succeeded = true;
		if (&element == vertexElement)
		{
			succeeded = Ply::ReadElement(input, header.format, element, vertexRequested, {}, vertexData, &error);
		}
		else if (&element == faceElement)
		{
			const int indexProperty = FindAny(element.properties, { "vertex_indices", "vertex_index" });
			succeeded = Ply::ReadElement(input, header.format, element, {}, { indexProperty }, faceData, &error);
		}
		else
		{
			succeeded = Ply::SkipElement(input, header.format, element, &error);
		}

		if (!succeeded)
		{
			std::cerr << error << '\n';
			out.Clear();
			return false;
		}
	}

	out.positions.reserve(vertexData.recordCount);
	for (std::size_t record = 0; record < vertexData.recordCount; ++record)
	{
		out.positions.push_back(XMFLOAT3(
			static_cast<float>(vertexData.Scalar(record, PositionX)),
			static_cast<float>(vertexData.Scalar(record, PositionY)),
			static_cast<float>(vertexData.Scalar(record, PositionZ))));

		if (hasNormals)
		{
			out.normals.push_back(XMFLOAT3(
				static_cast<float>(vertexData.Scalar(record, NormalX)),
				static_cast<float>(vertexData.Scalar(record, NormalY)),
				static_cast<float>(vertexData.Scalar(record, NormalZ))));
		}

		if (hasTexcoords)
		{
			out.texcoords.push_back(XMFLOAT2(
				static_cast<float>(vertexData.Scalar(record, TexU)),
				static_cast<float>(vertexData.Scalar(record, TexV))));
		}
	}

	const UINT vertexCount = static_cast<UINT>(out.positions.size());
	for (std::size_t record = 0; record < faceData.recordCount; ++record)
	{
		const std::vector<std::uint32_t>& corners = faceData.List(record, 0);
		if (corners.size() < 3)
			continue;

		bool valid = true;
		for (std::uint32_t corner : corners)
			valid = valid && corner < vertexCount;
		if (!valid)
			continue;

		out.faces.emplace_back(std::vector<UINT>(corners.begin(), corners.end()));

		// Triangle fan, which is correct for the convex polygons PLY files contain.
		for (std::size_t corner = 1; corner + 1 < corners.size(); ++corner)
		{
			out.indices.push_back(corners[0]);
			out.indices.push_back(corners[corner]);
			out.indices.push_back(corners[corner + 1]);
		}
	}

	if (!hasNormals)
		ComputeSmoothNormals(out);

	out.BuildVertices();
	return !out.vertices.empty();
}
