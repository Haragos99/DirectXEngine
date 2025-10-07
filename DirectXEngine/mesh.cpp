#include "mesh.h"

// Convert .obj index (1-based or negative) to 0-based
static int fixIndex(int idx, int count) 
{
	return idx > 0 ? idx - 1 : count + idx;
}


bool Mesh::loadMesh(const std::string path)\
{
	std::ifstream in(path);
	if (!in.is_open()) 
	{
		std::cerr << "Failed to open OBJ: " << path << '\n';
		return false;
	}


	std::vector<DirectX::XMFLOAT3> temp_positions;
	std::vector<DirectX::XMFLOAT2> temp_texcoords;
	std::vector<DirectX::XMFLOAT3> temp_normals;
	std::unordered_map<VertexKey, unsigned int, VertexKeyHash> uniqueVertices;
	int cointer = 0;
	std::string line;
	while (std::getline(in, line)) 
	{
		cointer++;
		// trim leading spaces
		size_t p = line.find_first_not_of(" \t\r\n");
		if (p == std::string::npos)
		{
			continue; // empty line
		}
		if (line[p] == '#')
		{
			continue; // comment
		}
		std::istringstream iss(line.substr(p));
		std::string tag;
		iss >> tag;
		auto c = tag.c_str();
		if (tag == "v") 
		{
			float x, y, z; iss >> x >> y >> z;
			temp_positions.push_back({ x,y,z });
		}
		else if (tag == "vt") 
		{
			float u, v; iss >> u >> v;
			temp_texcoords.push_back({ u,v });
		}
		else if (tag == "vn") 
		{
			float x, y, z; iss >> x >> y >> z;
			temp_normals.push_back({ x,y,z });
		}
		else if (tag == "f") 
		{
				
			std::vector<unsigned int> faceIndices;
			std::string vert;

			while (iss >> vert) 
			{
				VertexKey key = { -1, -1, -1 };
				int v = -1, vt = -1, vn = -1;

				// parse v/vt/vn or v//vn or v/vt
				size_t firstSlash = vert.find('/');
				size_t secondSlash = vert.find('/', firstSlash + 1);
				if (firstSlash == std::string::npos) {
					// only vertex index
					v = std::stoi(vert);
				}
				else if (secondSlash == std::string::npos) 
				{
					// vertex and texcoord
					v = std::stoi(vert.substr(0, firstSlash));
					vt = std::stoi(vert.substr(firstSlash + 1));
				}
				else 
				{
					// vertex, texcoord, normal or vertex, normal
					v = std::stoi(vert.substr(0, firstSlash));
					if (secondSlash > firstSlash + 1)
					{
						vt = std::stoi(vert.substr(firstSlash + 1, secondSlash - firstSlash - 1));
					}
					vn = std::stoi(vert.substr(secondSlash + 1));
				}
				
				key.v = fixIndex(v, (int)temp_positions.size());
				key.vt = vt != -1 ? fixIndex(vt, (int)temp_texcoords.size()) : -1;
				key.vn = vn != -1 ? fixIndex(vn, (int)temp_normals.size()) : -1;


				// Add vertex if not seen before
				auto it = uniqueVertices.find(key);
				if (it == uniqueVertices.end()) {
					unsigned int newIndex = (unsigned int)(positions.size());

					// Add position
					auto& p = temp_positions[key.v];
					positions.insert(positions.end(), { p.x, p.y, p.z });

					// Add texcoord
					if (key.vt >= 0) 
					{
						auto& t = temp_texcoords[key.vt];
						texcoords.insert(texcoords.end(), { t.x, t.y });
					}
					else {
						texcoords.insert(texcoords.end(), { 0.0f, 0.0f });
					}

					// Add normal
					if (key.vn >= 0) {
						auto& n = temp_normals[key.vn];
						normals.insert(normals.end(), { n.x, n.y, n.z });
					}
					else {
						normals.insert(normals.end(), { 0.0f, 0.0f, 0.0f });
					}

					uniqueVertices[key] = newIndex;
					faceIndices.push_back(newIndex);
				}
				else {
					faceIndices.push_back(it->second);
				}
			}
			// Triangulate using fan method
			if (faceIndices.size() >= 3) {
				for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
					indices.push_back(faceIndices[0]);
					indices.push_back(faceIndices[i + 1]);
					indices.push_back(faceIndices[i]);
					faces.push_back(FaceHandle(faceIndices[0], faceIndices[i+1], faceIndices[i]));
				}
			}

		}
	}
	initvertices();
}


void Mesh::initvertices()
{
	vertices.resize(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) 
	{
		vertices[i].position = { positions[i] };
		vertices[i].texcoord = { texcoords[i] };
		vertices[i].normal = { normals[i] };
	}
}



void Mesh::ComputeTangents()
{
	using namespace DirectX;

	size_t numVertices = vertices.size();
	size_t numFaces = faces.size();

	tangents.assign(numVertices, XMFLOAT3(0, 0, 0));


	for (size_t i = 0; i < numFaces; ++i)
	{
		FaceHandle& face = faces[i];
		XMFLOAT3 faceTangent = face.computeFaceTangent(positions, normals);
		
		// Add to each vertex in this face
		XMVECTOR t = XMLoadFloat3(&faceTangent);
		for (int vidx : face.vertexIndices)
		{
			XMVECTOR existing = XMLoadFloat3(&tangents[vidx]);
			XMVECTOR sum = XMVectorAdd(existing, t);
			XMStoreFloat3(&tangents[vidx], sum);
		}
	}

	// --- 2️⃣ Normalize all vertex tangents ---
	for (size_t i = 0; i < numVertices; ++i)
	{
		XMVECTOR t = XMLoadFloat3(&tangents[i]);
		t = XMVector3Normalize(t);
		XMStoreFloat3(&tangents[i], t);
	}
}