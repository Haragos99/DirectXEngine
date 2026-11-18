#pragma once
#include "ipanelsection.h"

#include <cstdint>
#include <vector>

class HalfEdgeMesh;
class SkinWeightTable;
class SkinningController;
struct SkinInfluence;

class SkinWeightsSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Skin Weights"; }
	bool IsVisible(const UIState& state) const override;
	void Draw(UIState& state) override;

private:
	struct CacheKey
	{
		const void* mesh = nullptr;
		const SkinWeightTable* weights = nullptr;
		uint64_t revision = 0;
		bool welded = false;
		bool hideEmptyBones = false;
		bool blendedOnly = false;

		bool operator==(const CacheKey&) const = default;
	};

	static constexpr unsigned kVertexColumnId = 0;
	static constexpr unsigned kTotalColumnId = 0xFFFFu;

	void Rebuild(const CacheKey& key, const SkinWeightTable& weights, const HalfEdgeMesh* topology);
	void Sort(const SkinWeightTable& weights);
	size_t SourceVertex(int row) const;
	void DrawTable(SkinningController& skinning, const SkinWeightTable& weights,
	               const std::vector<SkinInfluence>& influences);

	bool welded = false;
	bool hideEmptyBones = true;
	bool blendedOnly = false;
	int jumpTarget = 0;
	bool jumpRequested = false;

	CacheKey cacheKey;
	bool cacheValid = false;
	std::vector<size_t> columns;
	std::vector<int> rows;
	std::vector<int> representative;
	bool weldedAvailable = false;

	unsigned sortColumn = kVertexColumnId;
	bool sortDescending = false;

	int selectedRow = -1;
};
