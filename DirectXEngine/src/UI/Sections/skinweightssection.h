#pragma once
#include "ipanelsection.h"

#include <cstdint>
#include <vector>

class HalfEdgeMesh;
class SkinWeightTable;
class SkinningController;
struct SkinInfluence;

// Every vertex's weight per bone of the selected, bound mesh, as a table: one
// row per vertex, one column per bone tinted in its colour, and the row total.
// Drawn in a window of its own because the control panel is too narrow.
//
// Only the visible rows are drawn, and the row list (filtered and sorted) is
// rebuilt only when the weights are re-solved or a filter changes, so meshes
// with 100k vertices stay cheap.
class SkinWeightsSection : public IPanelSection
{
public:
	const char* GetTitle() const override { return "Skin Weights"; }
	bool IsVisible(const UIState& state) const override;
	void Draw(UIState& state) override;

private:
	// What the cached row list was built from; any change rebuilds it.
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

	// Column user ids: the vertex column, then 1 + influence index, then the total.
	static constexpr unsigned kVertexColumnId = 0;
	static constexpr unsigned kTotalColumnId = 0xFFFFu;

	void Rebuild(const CacheKey& key, const SkinWeightTable& weights, const HalfEdgeMesh* topology);
	void Sort(const SkinWeightTable& weights);
	// Render vertex whose weights row `row` shows.
	size_t SourceVertex(int row) const;
	void DrawTable(SkinningController& skinning, const SkinWeightTable& weights,
	               const std::vector<SkinInfluence>& influences);

	// Filters.
	bool welded = false;
	bool hideEmptyBones = true;
	bool blendedOnly = false;
	int jumpTarget = 0;
	bool jumpRequested = false;

	CacheKey cacheKey;
	bool cacheValid = false;
	std::vector<size_t> columns;   // influence indices shown, in order
	std::vector<int> rows;         // render or welded vertex ids, filtered and sorted
	std::vector<int> representative; // welded vertex -> first render vertex on it
	bool weldedAvailable = false;

	// Sort as last requested, re-applied whenever the rows are rebuilt.
	unsigned sortColumn = kVertexColumnId;
	bool sortDescending = false;

	int selectedRow = -1; // vertex id last clicked or jumped to
};
