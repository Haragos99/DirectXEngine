#pragma once
#include "vertex.h"
#include <vector>
#include <d3d11.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

// Unique key for a combination of v/vt/vn indices
struct VertexKey {
	int v, vt, vn;
	bool operator==(const VertexKey& other) const {
		return v == other.v && vt == other.vt && vn == other.vn;
	}
};

// Hash function for VertexKey
struct VertexKeyHash {
	std::size_t operator()(const VertexKey& key) const {
		return ((key.v * 73856093) ^ (key.vt * 19349663) ^ (key.vn * 83492791));
	}
};


struct FaceHandle {
public:
	std::vector<UINT> vertexIndices;
	FaceHandle(const std::vector<UINT>& verts) : vertexIndices(verts) {}
	FaceHandle(UINT v1, UINT v2, UINT v3) { vertexIndices = { v1, v2, v3 }; }
	// Return number of vertices
	size_t size() const { return vertexIndices.size(); }

	// Access vertex index
	int operator[](size_t i) const 
	{
		assert(i < vertexIndices.size());
		return vertexIndices[i];
	}
	// Check if vertex is part of this face
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
	// Equality comparison (unordered)
	bool operator==(const FaceHandle& other) const {
		if (vertexIndices.size() != other.vertexIndices.size()) return false;
		for (int v : vertexIndices) {
			if (!other.contains(v))
				return false;
		}
		return true;
	}

	DirectX::XMFLOAT3 computeFaceTangent(
		const std::vector<DirectX::XMFLOAT3>& vertices,
		const std::vector<DirectX::XMFLOAT3>& normals)
	{
		
		DirectX::XMVECTOR v0 = XMLoadFloat3(&vertices[vertexIndices[0]]);
		DirectX::XMVECTOR v1 = XMLoadFloat3(&vertices[vertexIndices[1]]);
		DirectX::XMVECTOR v2 = XMLoadFloat3(&vertices[vertexIndices[2]]);

		DirectX::XMVECTOR n0 = XMLoadFloat3(&normals[vertexIndices[0]]);
		DirectX::XMVECTOR n1 = XMLoadFloat3(&normals[vertexIndices[1]]);
		DirectX::XMVECTOR n2 = XMLoadFloat3(&normals[vertexIndices[2]]);

		// Average normal for smoother tangent
		DirectX::XMVECTOR normal = DirectX::XMVector3Normalize(DirectX::XMVectorAdd(DirectX::XMVectorAdd(n0, n1), n2));

		// Use one edge as base
		DirectX::XMVECTOR edge = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(v1, v0));

		// Project edge onto tangent plane
		DirectX::XMVECTOR dotNE = DirectX::XMVector3Dot(edge, normal);
		DirectX::XMVECTOR tangent = DirectX::XMVectorSubtract(edge, DirectX::XMVectorMultiply(normal, dotNE));
		tangent = DirectX::XMVector3Normalize(tangent);

		// Store result
		DirectX::XMFLOAT3 result;
		DirectX::XMStoreFloat3(&result, tangent);
		return result;
	}

};



class Mesh
{
public :
		Mesh() = default;
		~Mesh() = default;
		std::vector<VertexData> vertices;
		std::vector<UINT>   indices;
		std::vector<FaceHandle> faces;
		bool loadMesh(const std::string path);
		void ComputeTangents();
private:
		std::vector<DirectX::XMFLOAT3> positions;
		std::vector<DirectX::XMFLOAT3> normals; 
		std::vector<DirectX::XMFLOAT2> texcoords; 
		std::vector<DirectX::XMFLOAT3> tangents;
		void initvertices();
};