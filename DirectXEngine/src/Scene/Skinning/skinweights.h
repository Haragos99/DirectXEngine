#pragma once
#include <cstddef>
#include <vector>


class SkinWeightTable
{
public:
	void Resize(size_t vertexCount, size_t jointCount)
	{
		weights.assign(vertexCount, std::vector<float>(jointCount, 0.0f));
		joints = jointCount;
	}

	size_t VertexCount() const { return weights.size(); }
	size_t JointCount() const { return joints; }
	bool IsEmpty() const { return weights.empty() || joints == 0; }

	std::vector<float>& operator[](size_t vertex) { return weights[vertex]; }
	const std::vector<float>& operator[](size_t vertex) const { return weights[vertex]; }

	// Row major, matching how gWeights is indexed in the skinning shaders:
	// vertex v owns [v * jointCount, +jointCount).
	std::vector<float> Flatten() const
	{
		std::vector<float> flat;
		flat.reserve(weights.size() * joints);
		for (const std::vector<float>& row : weights)
			flat.insert(flat.end(), row.begin(), row.end());
		return flat;
	}

private:
	std::vector<std::vector<float>> weights;
	// Kept separately so the joint count survives an empty table.
	size_t joints = 0;
};
