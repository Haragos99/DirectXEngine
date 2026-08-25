#pragma once
#include "vertex.h"
#include <DirectXMath.h>
#include <cassert>
#include <string>
#include <vector>
#include <d3d11.h>

// A polygon expressed as indices into MeshData's vertex arrays.
struct FaceHandle
{
	std::vector<UINT> vertexIndices;

	FaceHandle(const std::vector<UINT>& verts) : vertexIndices(verts) {}
	FaceHandle(UINT v1, UINT v2, UINT v3) { vertexIndices = { v1, v2, v3 }; }

	size_t size() const { return vertexIndices.size(); }

	int operator[](size_t i) const
	{
		assert(i < vertexIndices.size());
		return vertexIndices[i];
	}

	bool contains(int vertexIdx) const
	{
		for (int v : vertexIndices)
		{
			if (v == vertexIdx)
			{
				return true;
			}
		}
		return false;
	}

	// Unordered comparison: two faces match when they share the same vertex set.
	bool operator==(const FaceHandle& other) const
	{
		if (vertexIndices.size() != other.vertexIndices.size()) return false;
		for (int v : vertexIndices)
		{
			if (!other.contains(v))
				return false;
		}
		return true;
	}

	DirectX::XMFLOAT3 computeFaceTangent(
		const std::vector<DirectX::XMFLOAT3>& vertices,
		const std::vector<DirectX::XMFLOAT3>& normals) const;
};

// Format-independent geometry produced by an IModelLoader implementation.
// Loaders fill the raw attribute arrays plus indices/faces and then call
// BuildVertices() to produce the interleaved vertex buffer contents.
struct MeshData
{
	std::vector<DirectX::XMFLOAT3> positions;
	std::vector<DirectX::XMFLOAT3> normals;
	std::vector<DirectX::XMFLOAT2> texcoords;
	std::vector<VertexData> vertices;
	std::vector<UINT> indices;
	std::vector<FaceHandle> faces;

	void Clear();
	// Interleave positions/normals/texcoords into `vertices`.
	void BuildVertices();
};

// Abstraction over a model file format. Add a new format by implementing this
// interface and registering it with ModelLoaderRegistry - no existing code has
// to change.
class IModelLoader
{
public:
	virtual ~IModelLoader() = default;

	// Human readable format name shown in file dialogs, e.g. "Wavefront OBJ".
	virtual const char* GetFormatName() const = 0;
	// Lowercase extensions handled by this loader, without the dot, e.g. {"obj"}.
	virtual std::vector<std::string> GetSupportedExtensions() const = 0;
	// True when this loader handles the given lowercase extension.
	virtual bool CanLoad(const std::string& extension) const;
	// Parse `path` into `out`. Returns false when the file cannot be read.
	virtual bool Load(const std::string& path, MeshData& out) const = 0;
};
