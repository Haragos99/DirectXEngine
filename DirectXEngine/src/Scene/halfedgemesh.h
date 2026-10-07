#pragma once
#include "vertex.h"

#include <DirectXMath.h>
#include <cstdint>
#include <vector>

// Half-edge connectivity for a triangle mesh.
//
// Built on WELDED positions: importers split a vertex wherever the uv or the
// normal changes, so one position arrives under several render indices. Walking
// the render indices would leave a hole along every uv seam, so the topology is
// welded and `VertexOf` maps a render vertex onto its topological one.
//
// Every interior half-edge is paired with an opposite. Unmatched ones get a
// boundary half-edge (face == kInvalid) linked into a loop, so `Opposite` is
// always valid and the circulators always terminate.
class HalfEdgeMesh
{
public:
	static constexpr int kInvalid = -1;

	struct HalfEdge
	{
		int origin = kInvalid;
		int target = kInvalid;
		int next = kInvalid;
		int prev = kInvalid;
		int opposite = kInvalid;
		int face = kInvalid; // kInvalid on a boundary half-edge
	};

	bool Build(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices);
	void Clear();
	bool IsEmpty() const { return halfEdges.empty(); }

	size_t VertexCount() const { return positions.size(); }
	size_t HalfEdgeCount() const { return halfEdges.size(); }
	size_t FaceCount() const { return faceHalfEdge.size(); }

	const DirectX::XMFLOAT3& Position(int vertex) const { return positions[vertex]; }
	// Render vertex -> topological vertex.
	int VertexOf(size_t renderVertex) const { return vertexMap[renderVertex]; }
	const std::vector<int>& VertexMap() const { return vertexMap; }
	size_t RenderVertexCount() const { return vertexMap.size(); }

	const HalfEdge& Edge(int halfEdge) const { return halfEdges[halfEdge]; }
	int Origin(int halfEdge) const { return halfEdges[halfEdge].origin; }
	int Target(int halfEdge) const { return halfEdges[halfEdge].target; }
	int Next(int halfEdge) const { return halfEdges[halfEdge].next; }
	int Prev(int halfEdge) const { return halfEdges[halfEdge].prev; }
	int Opposite(int halfEdge) const { return halfEdges[halfEdge].opposite; }
	int Face(int halfEdge) const { return halfEdges[halfEdge].face; }
	bool IsBoundary(int halfEdge) const { return halfEdges[halfEdge].face == kInvalid; }

	int OutgoingHalfEdge(int vertex) const { return vertexHalfEdge[vertex]; }
	int FaceHalfEdge(int face) const { return faceHalfEdge[face]; }
	int FindHalfEdge(int from, int to) const;
	int Valence(int vertex) const;
	bool IsBoundaryVertex(int vertex) const;

	// Interior angle of the face at Target(halfEdge), i.e. between the outgoing
	// next edge and the incoming one. Matches OpenMesh's calc_sector_angle.
	double SectorAngle(int halfEdge) const;

	// --- circulators -------------------------------------------------------
	// Projections decide what a circulator yields; skipInvalid drops the
	// boundary entries that have no face.
	struct YieldHalfEdge
	{
		static constexpr bool skipInvalid = false;
		static int Of(const HalfEdgeMesh&, int halfEdge) { return halfEdge; }
	};
	struct YieldTarget
	{
		static constexpr bool skipInvalid = false;
		static int Of(const HalfEdgeMesh& mesh, int halfEdge) { return mesh.Target(halfEdge); }
	};
	struct YieldFace
	{
		static constexpr bool skipInvalid = true;
		static int Of(const HalfEdgeMesh& mesh, int halfEdge) { return mesh.Face(halfEdge); }
	};
	struct YieldOppositeFace
	{
		static constexpr bool skipInvalid = true;
		static int Of(const HalfEdgeMesh& mesh, int halfEdge) { return mesh.Face(mesh.Opposite(halfEdge)); }
	};

	// Walks the outgoing half-edges around a vertex.
	template <typename Yield>
	class VertexRange
	{
	public:
		class Iterator
		{
		public:
			Iterator(const HalfEdgeMesh* _mesh, int _start, bool done)
				: mesh(_mesh), start(_start), current(_start), finished(done)
			{
				SkipInvalid();
			}

			int operator*() const { return Yield::Of(*mesh, current); }
			bool operator!=(const Iterator& other) const { return finished != other.finished; }

			Iterator& operator++()
			{
				Advance();
				SkipInvalid();
				return *this;
			}

		private:
			void Advance()
			{
				current = mesh->Next(mesh->Opposite(current));
				if (current == start)
					finished = true;
			}

			void SkipInvalid()
			{
				if (!Yield::skipInvalid)
					return;
				while (!finished && Yield::Of(*mesh, current) == kInvalid)
					Advance();
			}

			const HalfEdgeMesh* mesh;
			int start;
			int current;
			bool finished;
		};

		VertexRange(const HalfEdgeMesh& _mesh, int vertex)
			: mesh(&_mesh), start(_mesh.OutgoingHalfEdge(vertex))
		{
		}

		Iterator begin() const { return Iterator(mesh, start, start == kInvalid); }
		Iterator end() const { return Iterator(mesh, start, true); }

	private:
		const HalfEdgeMesh* mesh;
		int start;
	};

	// Walks the half-edges around a face.
	template <typename Yield>
	class FaceRange
	{
	public:
		class Iterator
		{
		public:
			Iterator(const HalfEdgeMesh* _mesh, int _start, bool done)
				: mesh(_mesh), start(_start), current(_start), finished(done)
			{
				SkipInvalid();
			}

			int operator*() const { return Yield::Of(*mesh, current); }
			bool operator!=(const Iterator& other) const { return finished != other.finished; }

			Iterator& operator++()
			{
				Advance();
				SkipInvalid();
				return *this;
			}

		private:
			void Advance()
			{
				current = mesh->Next(current);
				if (current == start)
					finished = true;
			}

			void SkipInvalid()
			{
				if (!Yield::skipInvalid)
					return;
				while (!finished && Yield::Of(*mesh, current) == kInvalid)
					Advance();
			}

			const HalfEdgeMesh* mesh;
			int start;
			int current;
			bool finished;
		};

		FaceRange(const HalfEdgeMesh& _mesh, int face)
			: mesh(&_mesh), start(_mesh.FaceHalfEdge(face))
		{
		}

		Iterator begin() const { return Iterator(mesh, start, start == kInvalid); }
		Iterator end() const { return Iterator(mesh, start, true); }

	private:
		const HalfEdgeMesh* mesh;
		int start;
	};

	VertexRange<YieldTarget> Neighbours(int vertex) const { return { *this, vertex }; }
	VertexRange<YieldHalfEdge> OutgoingHalfEdges(int vertex) const { return { *this, vertex }; }
	VertexRange<YieldFace> VertexFaces(int vertex) const { return { *this, vertex }; }
	FaceRange<YieldHalfEdge> FaceHalfEdges(int face) const { return { *this, face }; }
	FaceRange<YieldTarget> FaceVertices(int face) const { return { *this, face }; }
	FaceRange<YieldOppositeFace> FaceNeighbours(int face) const { return { *this, face }; }

private:
	std::vector<DirectX::XMFLOAT3> positions;
	std::vector<int> vertexMap;       // render vertex -> welded vertex
	std::vector<int> vertexHalfEdge;  // welded vertex -> an outgoing half-edge
	std::vector<int> faceHalfEdge;
	std::vector<HalfEdge> halfEdges;
};
