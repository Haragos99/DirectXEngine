#include "objmodelloader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace
{
	// Unique key for a v/vt/vn index combination, used to deduplicate vertices.
	struct VertexKey
	{
		int v, vt, vn;
		bool operator==(const VertexKey& other) const
		{
			return v == other.v && vt == other.vt && vn == other.vn;
		}
	};

	struct VertexKeyHash
	{
		std::size_t operator()(const VertexKey& key) const
		{
			return ((key.v * 73856093) ^ (key.vt * 19349663) ^ (key.vn * 83492791));
		}
	};

	// Convert an .obj index (1-based or negative) to 0-based.
	int fixIndex(int idx, int count)
	{
		return idx > 0 ? idx - 1 : count + idx;
	}

	// Parse a single "v", "v/vt", "v//vn" or "v/vt/vn" face element.
	VertexKey parseFaceElement(const std::string& element,
		size_t positionCount, size_t texcoordCount, size_t normalCount)
	{
		int v = -1, vt = -1, vn = -1;

		const size_t firstSlash = element.find('/');
		const size_t secondSlash = element.find('/', firstSlash + 1);
		if (firstSlash == std::string::npos)
		{
			v = std::stoi(element);
		}
		else if (secondSlash == std::string::npos)
		{
			v = std::stoi(element.substr(0, firstSlash));
			vt = std::stoi(element.substr(firstSlash + 1));
		}
		else
		{
			v = std::stoi(element.substr(0, firstSlash));
			if (secondSlash > firstSlash + 1)
				vt = std::stoi(element.substr(firstSlash + 1, secondSlash - firstSlash - 1));
			vn = std::stoi(element.substr(secondSlash + 1));
		}

		VertexKey key{};
		key.v = fixIndex(v, static_cast<int>(positionCount));
		key.vt = vt != -1 ? fixIndex(vt, static_cast<int>(texcoordCount)) : -1;
		key.vn = vn != -1 ? fixIndex(vn, static_cast<int>(normalCount)) : -1;
		return key;
	}
}

bool ObjModelLoader::Load(const std::string& path, MeshData& out) const
{
	std::ifstream in(path);
	if (!in.is_open())
	{
		std::cerr << "Failed to open OBJ: " << path << '\n';
		return false;
	}

	out.Clear();

	std::vector<DirectX::XMFLOAT3> filePositions;
	std::vector<DirectX::XMFLOAT2> fileTexcoords;
	std::vector<DirectX::XMFLOAT3> fileNormals;
	std::unordered_map<VertexKey, UINT, VertexKeyHash> uniqueVertices;

	std::string line;
	while (std::getline(in, line))
	{
		const size_t start = line.find_first_not_of(" \t\r\n");
		if (start == std::string::npos || line[start] == '#')
			continue;

		std::istringstream iss(line.substr(start));
		std::string tag;
		iss >> tag;

		if (tag == "v")
		{
			float x, y, z; iss >> x >> y >> z;
			filePositions.push_back({ x, y, z });
		}
		else if (tag == "vt")
		{
			float u, v; iss >> u >> v;
			fileTexcoords.push_back({ u, v });
		}
		else if (tag == "vn")
		{
			float x, y, z; iss >> x >> y >> z;
			fileNormals.push_back({ x, y, z });
		}
		else if (tag == "f")
		{
			std::vector<UINT> faceIndices;
			std::string element;
			while (iss >> element)
			{
				const VertexKey key = parseFaceElement(element,
					filePositions.size(), fileTexcoords.size(), fileNormals.size());

				if (key.v < 0 || key.v >= static_cast<int>(filePositions.size()))
					continue;

				auto it = uniqueVertices.find(key);
				if (it != uniqueVertices.end())
				{
					faceIndices.push_back(it->second);
					continue;
				}

				const UINT newIndex = static_cast<UINT>(out.positions.size());
				out.positions.push_back(filePositions[key.v]);
				out.texcoords.push_back(key.vt >= 0 && key.vt < static_cast<int>(fileTexcoords.size())
					? fileTexcoords[key.vt]
					: DirectX::XMFLOAT2(0.0f, 0.0f));
				out.normals.push_back(key.vn >= 0 && key.vn < static_cast<int>(fileNormals.size())
					? fileNormals[key.vn]
					: DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));

				uniqueVertices[key] = newIndex;
				faceIndices.push_back(newIndex);
			}

			// Triangulate n-gons with a fan around the first vertex.
			for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
			{
				out.indices.push_back(faceIndices[0]);
				out.indices.push_back(faceIndices[i + 1]);
				out.indices.push_back(faceIndices[i]);
				out.faces.push_back(FaceHandle(faceIndices[0], faceIndices[i + 1], faceIndices[i]));
			}
		}
	}

	out.BuildVertices();
	return true;
}
