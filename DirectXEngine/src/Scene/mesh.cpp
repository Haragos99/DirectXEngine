#include "mesh.h"
#include "modelloaderregistry.h"

using namespace DirectX;

bool Mesh::loadMesh(const std::string& path)
{
	return ModelLoaderRegistry::Instance().Load(path, data);
}

bool Mesh::loadMesh(const std::string& path, const IModelLoader& loader)
{
	return loader.Load(path, data);
}

void Mesh::ComputeTangents()
{
	tangents.assign(data.vertices.size(), XMFLOAT3(0.0f, 0.0f, 0.0f));

	for (const FaceHandle& face : data.faces)
	{
		const XMFLOAT3 faceTangent = face.computeFaceTangent(data.positions, data.normals);
		const XMVECTOR t = XMLoadFloat3(&faceTangent);
		for (UINT vertexIndex : face.vertexIndices)
		{
			const XMVECTOR existing = XMLoadFloat3(&tangents[vertexIndex]);
			XMStoreFloat3(&tangents[vertexIndex], XMVectorAdd(existing, t));
		}
	}

	for (XMFLOAT3& tangent : tangents)
	{
		XMStoreFloat3(&tangent, XMVector3Normalize(XMLoadFloat3(&tangent)));
	}
}
