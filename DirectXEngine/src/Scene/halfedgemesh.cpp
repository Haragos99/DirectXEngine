#include "halfedgemesh.h"

#include <cmath>
#include <cstring>
#include <unordered_map>

using namespace DirectX;

namespace
{
	// Importers copy a split vertex from one source position, so duplicates are
	// bit identical and an exact key welds them with no epsilon to tune.
	struct PositionKey
	{
		uint32_t x = 0;
		uint32_t y = 0;
		uint32_t z = 0;

		explicit PositionKey(const XMFLOAT3& position)
			: x(Bits(position.x)), y(Bits(position.y)), z(Bits(position.z))
		{
		}

		bool operator==(const PositionKey& other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}

	private:
		static uint32_t Bits(float value)
		{
			// -0.0f and 0.0f compare equal but have different bit patterns.
			const float normalised = value == 0.0f ? 0.0f : value;
			uint32_t bits = 0;
			std::memcpy(&bits, &normalised, sizeof(bits));
			return bits;
		}
	};

	struct PositionKeyHash
	{
		size_t operator()(const PositionKey& key) const
		{
			size_t hash = key.x;
			hash = hash * 31u + key.y;
			hash = hash * 31u + key.z;
			return hash;
		}
	};

	uint64_t DirectedEdgeKey(int from, int to)
	{
		return (static_cast<uint64_t>(static_cast<uint32_t>(from)) << 32)
		     | static_cast<uint32_t>(to);
	}
}

void HalfEdgeMesh::Clear()
{
	positions.clear();
	vertexMap.clear();
	vertexHalfEdge.clear();
	faceHalfEdge.clear();
	halfEdges.clear();
}

bool HalfEdgeMesh::Build(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices)
{
	Clear();
	if (vertices.empty() || indices.size() < 3)
	{
		return false;
	}

	// 1. Weld positions so the topology is continuous across uv seams.
	std::unordered_map<PositionKey, int, PositionKeyHash> unique;
	vertexMap.resize(vertices.size());
	for (size_t v = 0; v < vertices.size(); ++v)
	{
		const PositionKey key(vertices[v].position);
		const auto existing = unique.find(key);
		if (existing != unique.end())
		{
			vertexMap[v] = existing->second;
			continue;
		}

		const int welded = static_cast<int>(positions.size());
		unique.emplace(key, welded);
		positions.push_back(vertices[v].position);
		vertexMap[v] = welded;
	}
	vertexHalfEdge.assign(positions.size(), kInvalid);

	// 2. One half-edge per triangle corner.
	std::unordered_map<uint64_t, int> directed;
	for (size_t triangle = 0; triangle + 2 < indices.size(); triangle += 3)
	{
		const int corner[3] = {
			vertexMap[indices[triangle]],
			vertexMap[indices[triangle + 1]],
			vertexMap[indices[triangle + 2]],
		};

		if (corner[0] == corner[1] || corner[1] == corner[2] || corner[0] == corner[2])
		{
			continue; // collapsed once welded
		}

		// A directed edge already in use means a non-manifold fan; dropping the
		// duplicate keeps the structure walkable instead of corrupting it.
		bool usable = true;
		for (int k = 0; k < 3 && usable; ++k)
		{
			usable = directed.find(DirectedEdgeKey(corner[k], corner[(k + 1) % 3])) == directed.end();
		}
		if (!usable)
		{
			continue;
		}

		const int face = static_cast<int>(faceHalfEdge.size());
		const int base = static_cast<int>(halfEdges.size());
		for (int k = 0; k < 3; ++k)
		{
			HalfEdge edge;
			edge.origin = corner[k];
			edge.target = corner[(k + 1) % 3];
			edge.next = base + (k + 1) % 3;
			edge.prev = base + (k + 2) % 3;
			edge.face = face;
			halfEdges.push_back(edge);

			directed.emplace(DirectedEdgeKey(edge.origin, edge.target), base + k);
			if (vertexHalfEdge[edge.origin] == kInvalid)
			{
				vertexHalfEdge[edge.origin] = base + k;
			}
		}
		faceHalfEdge.push_back(base);
	}

	if (halfEdges.empty())
	{
		Clear();
		return false;
	}

	// 3. Pair opposites.
	for (int h = 0; h < static_cast<int>(halfEdges.size()); ++h)
	{
		const auto twin = directed.find(DirectedEdgeKey(halfEdges[h].target, halfEdges[h].origin));
		if (twin != directed.end())
		{
			halfEdges[h].opposite = twin->second;
		}
	}

	// 4. Give every unmatched half-edge a boundary twin, so Opposite is always
	//    valid and the circulators cannot run off the edge of the surface.
	std::vector<int> boundaryByOrigin(positions.size(), kInvalid);
	const int interiorCount = static_cast<int>(halfEdges.size());
	for (int h = 0; h < interiorCount; ++h)
	{
		if (halfEdges[h].opposite != kInvalid)
		{
			continue;
		}

		HalfEdge boundary;
		boundary.origin = halfEdges[h].target;
		boundary.target = halfEdges[h].origin;
		boundary.opposite = h;
		boundary.face = kInvalid;

		const int index = static_cast<int>(halfEdges.size());
		halfEdges.push_back(boundary);
		halfEdges[h].opposite = index;
		boundaryByOrigin[boundary.origin] = index;

		// A boundary vertex must start its circulation on the boundary, or the
		// walk would stop before covering the whole fan.
		vertexHalfEdge[boundary.origin] = index;
	}

	// 5. Link the boundary half-edges into loops.
	for (int h = interiorCount; h < static_cast<int>(halfEdges.size()); ++h)
	{
		const int following = boundaryByOrigin[halfEdges[h].target];
		halfEdges[h].next = following;
		if (following != kInvalid)
		{
			halfEdges[following].prev = h;
		}
	}

	return true;
}

int HalfEdgeMesh::FindHalfEdge(int from, int to) const
{
	if (from < 0 || from >= static_cast<int>(vertexHalfEdge.size()))
	{
		return kInvalid;
	}

	for (int halfEdge : OutgoingHalfEdges(from))
	{
		if (halfEdges[halfEdge].target == to)
		{
			return halfEdge;
		}
	}
	return kInvalid;
}

int HalfEdgeMesh::Valence(int vertex) const
{
	int count = 0;
	for (int neighbour : Neighbours(vertex))
	{
		(void)neighbour;
		++count;
	}
	return count;
}

bool HalfEdgeMesh::IsBoundaryVertex(int vertex) const
{
	for (int halfEdge : OutgoingHalfEdges(vertex))
	{
		if (IsBoundary(halfEdge))
		{
			return true;
		}
	}
	return false;
}

double HalfEdgeMesh::SectorAngle(int halfEdge) const
{
	const int following = Next(halfEdge);
	if (following == kInvalid)
	{
		return 0.0;
	}

	// The angle sits at Target(halfEdge), between the outgoing next edge and the
	// edge this half-edge arrived along.
	const XMVECTOR apex = XMLoadFloat3(&positions[halfEdges[halfEdge].target]);
	const XMVECTOR toNext = XMVectorSubtract(XMLoadFloat3(&positions[halfEdges[following].target]), apex);
	const XMVECTOR toPrevious = XMVectorSubtract(XMLoadFloat3(&positions[halfEdges[halfEdge].origin]), apex);

	const double lengths = static_cast<double>(XMVectorGetX(XMVector3Length(toNext)))
	                     * static_cast<double>(XMVectorGetX(XMVector3Length(toPrevious)));
	if (lengths < 1e-20)
	{
		return 0.0;
	}

	double cosine = static_cast<double>(XMVectorGetX(XMVector3Dot(toNext, toPrevious))) / lengths;
	cosine = cosine < -1.0 ? -1.0 : (cosine > 1.0 ? 1.0 : cosine);
	return std::acos(cosine);
}
