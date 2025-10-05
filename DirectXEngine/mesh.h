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


class Mesh
{
	public :
		Mesh() = default;
		~Mesh() = default;
		std::vector<Vertex> vertices;
		std::vector<UINT>   indices;
		bool loadMesh(const std::string path);
private:
		std::vector<DirectX::XMFLOAT3> positions;
		std::vector<DirectX::XMFLOAT3> normals; 
		std::vector<DirectX::XMFLOAT2> texcoords; 
		void initvertices();
};