#pragma once
#include "halfedgemesh.h"
#include "imodelloader.h"
#include <string>
#include <vector>

// Geometry container for a loaded model. It owns the MeshData and delegates the
// actual file parsing to an IModelLoader, so it knows nothing about formats.
class Mesh
{
public:
	Mesh() = default;
	~Mesh() = default;

	// Load through the shared registry, picking the loader by file extension.
	bool loadMesh(const std::string& path);
	// Load through an explicitly supplied loader (dependency injection).
	bool loadMesh(const std::string& path, const IModelLoader& loader);

	const MeshData& GetData() const { return data; }
	const std::vector<VertexData>& GetVertices() const { return data.vertices; }
	const std::vector<UINT>& GetIndices() const { return data.indices; }
	const std::vector<FaceHandle>& GetFaces() const { return data.faces; }
	const std::vector<DirectX::XMFLOAT3>& GetTangents() const { return tangents; }
	// Connectivity, built once the geometry is in.
	const HalfEdgeMesh& GetHalfEdge() const { return halfEdge; }

	void ComputeTangents();

private:
	// Connectivity is rebuilt with the geometry, so it is never stale.
	bool BuildTopology(bool loaded);

	MeshData data;
	HalfEdgeMesh halfEdge;
	std::vector<DirectX::XMFLOAT3> tangents;
};