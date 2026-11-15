#pragma once
#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

class SkinWeightTable
{
public:
	void Resize(size_t vertexCount, size_t influenceCount)
	{
		vertices = vertexCount;
		influences = influenceCount;
		weights.assign(vertexCount * influenceCount, 0.0f);
	}

	size_t VertexCount() const { return vertices; }
	size_t InfluenceCount() const { return influences; }
	bool IsEmpty() const { return vertices == 0 || influences == 0; }

	std::span<float> Row(size_t vertex) { return { weights.data() + vertex * influences, influences }; }
	std::span<const float> Row(size_t vertex) const { return { weights.data() + vertex * influences, influences }; }
	std::span<float> operator[](size_t vertex) { return Row(vertex); }
	std::span<const float> operator[](size_t vertex) const { return Row(vertex); }

	void CopyRow(size_t vertex, const SkinWeightTable& from, size_t fromVertex)
	{
		std::ranges::copy(from.Row(fromVertex), Row(vertex).begin());
	}

	const std::vector<float>& Data() const { return weights; }

private:
	std::vector<float> weights;
	size_t vertices = 0;
	size_t influences = 0;
};
