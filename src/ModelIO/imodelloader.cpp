#include "imodelloader.h"

using namespace DirectX;

XMFLOAT3 FaceHandle::computeFaceTangent(
	const std::vector<XMFLOAT3>& vertices,
	const std::vector<XMFLOAT3>& normals) const
{
	XMVECTOR v0 = XMLoadFloat3(&vertices[vertexIndices[0]]);
	XMVECTOR v1 = XMLoadFloat3(&vertices[vertexIndices[1]]);

	XMVECTOR n0 = XMLoadFloat3(&normals[vertexIndices[0]]);
	XMVECTOR n1 = XMLoadFloat3(&normals[vertexIndices[1]]);
	XMVECTOR n2 = XMLoadFloat3(&normals[vertexIndices[2]]);

	// Average normal for a smoother tangent.
	XMVECTOR normal = XMVector3Normalize(XMVectorAdd(XMVectorAdd(n0, n1), n2));

	// Project one edge onto the tangent plane.
	XMVECTOR edge = XMVector3Normalize(XMVectorSubtract(v1, v0));
	XMVECTOR dotNE = XMVector3Dot(edge, normal);
	XMVECTOR tangent = XMVector3Normalize(XMVectorSubtract(edge, XMVectorMultiply(normal, dotNE)));

	XMFLOAT3 result;
	XMStoreFloat3(&result, tangent);
	return result;
}

void MeshData::Clear()
{
	positions.clear();
	normals.clear();
	texcoords.clear();
	vertices.clear();
	indices.clear();
	faces.clear();
}

void MeshData::BuildVertices()
{
	vertices.resize(positions.size());
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		vertices[i].position = positions[i];
		vertices[i].normal = i < normals.size() ? normals[i] : XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertices[i].texcoord = i < texcoords.size() ? texcoords[i] : XMFLOAT2(0.0f, 0.0f);
	}
}

bool IModelLoader::CanLoad(const std::string& extension) const
{
	for (const std::string& supported : GetSupportedExtensions())
	{
		if (supported == extension)
			return true;
	}
	return false;
}
