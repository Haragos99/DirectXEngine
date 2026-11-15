#include "skinweightssection.h"

#include "halfedgemesh.h"
#include "skeleton.h"
#include "skinbindingsection.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace DirectX;

namespace
{
	constexpr float kBlendedThreshold = 0.999f;
	constexpr float kTotalTolerance = 1e-3f;
	constexpr size_t kMaxInfluenceColumns = 512 - 2;
	constexpr float kWeightColumnWidth = 64.0f;

	const ImVec4 kWarningColor(0.95f, 0.45f, 0.35f, 1.0f);

	ImVec4 HeaderColor(const XMFLOAT3& color)
	{
		constexpr float lift = 0.35f;
		return ImVec4(color.x + (1.0f - color.x) * lift,
		              color.y + (1.0f - color.y) * lift,
		              color.z + (1.0f - color.z) * lift, 1.0f);
	}

	float RowTotal(std::span<const float> row)
	{
		float total = 0.0f;
		for (float weight : row)
			total += weight;
		return total;
	}
}

bool SkinWeightsSection::IsVisible(const UIState& state) const
{
	if (!state.skinWeightsWindowOpen)
	{
		return false;
	}

	const std::shared_ptr<Object3D> selected = state.SelectedObject();
	return selected && selected->GetSkinVertices() != nullptr;
}

size_t SkinWeightsSection::SourceVertex(int row) const
{
	return cacheKey.welded ? static_cast<size_t>(representative[row]) : static_cast<size_t>(row);
}

void SkinWeightsSection::Rebuild(const CacheKey& key, const SkinWeightTable& weights, const HalfEdgeMesh* topology)
{
	cacheKey = key;
	cacheValid = true;
	const size_t influenceCount = weights.InfluenceCount();

	representative.clear();
	if (key.welded && topology != nullptr)
	{
		representative.assign(topology->VertexCount(), -1);
		for (size_t v = 0; v < weights.VertexCount(); ++v)
		{
			int& slot = representative[topology->VertexOf(v)];
			if (slot < 0)
			{
				slot = static_cast<int>(v);
			}
		}
	}

	std::vector<bool> used(influenceCount, !key.hideEmptyBones);
	if (key.hideEmptyBones)
	{
		for (size_t v = 0; v < weights.VertexCount(); ++v)
		{
			const std::span<const float> row = weights.Row(v);
			for (size_t i = 0; i < influenceCount; ++i)
				used[i] = used[i] || row[i] > 0.0f;
		}
	}
	columns.clear();
	for (size_t i = 0; i < influenceCount; ++i)
	{
		if (used[i])
		{
			columns.push_back(i);
		}
	}

	rows.clear();
	const size_t rowCount = key.welded ? representative.size() : weights.VertexCount();
	for (size_t id = 0; id < rowCount; ++id)
	{
		if (key.welded && representative[id] < 0)
		{
			continue;
		}

		if (key.blendedOnly)
		{
			const std::span<const float> row = weights.Row(SourceVertex(static_cast<int>(id)));
			if (*std::max_element(row.begin(), row.end()) >= kBlendedThreshold)
			{
				continue;
			}
		}
		rows.push_back(static_cast<int>(id));
	}

	Sort(weights);
}

void SkinWeightsSection::Sort(const SkinWeightTable& weights)
{
	const unsigned column = sortColumn;
	const bool influenceColumn = column != kVertexColumnId && column != kTotalColumnId
	                          && column - 1 < weights.InfluenceCount();

	const auto value = [&](int id) -> float
	{
		const std::span<const float> row = weights.Row(SourceVertex(id));
		if (column == kTotalColumnId)
		{
			return RowTotal(row);
		}
		if (influenceColumn)
		{
			return row[column - 1];
		}
		return static_cast<float>(id);
	};

	std::stable_sort(rows.begin(), rows.end(), [&](int a, int b)
	{
		const float va = value(a);
		const float vb = value(b);
		if (va != vb)
		{
			return sortDescending ? va > vb : va < vb;
		}
		return a < b;
	});
}

void SkinWeightsSection::Draw(UIState& state)
{
	const std::shared_ptr<Object3D> mesh = state.SelectedObject();
	if (!mesh || state.sceneObjects == nullptr)
	{
		return;
	}

	const std::shared_ptr<Skeleton> rig = SkinBindingSection::FindBoundSkeleton(*state.sceneObjects, *mesh);
	if (!rig)
	{
		ImGui::TextDisabled("%s is not bound to a skeleton.", mesh->GetName().c_str());
		cacheValid = false;
		return;
	}

	SkinningController& skinning = rig->GetSkinning();
	const std::optional<SkinningController::BindingView> binding = skinning.GetBinding(*mesh);
	if (!binding)
	{
		return;
	}

	const SkinWeightTable& weights = binding->weights;
	const HalfEdgeMesh* topology = mesh->GetHalfEdgeMesh();
	weldedAvailable = topology != nullptr && !topology->IsEmpty()
	               && topology->RenderVertexCount() == weights.VertexCount();
	if (!weldedAvailable)
	{
		welded = false;
	}

	ImGui::Text("%s on %s: %zu vertices, %zu bones", mesh->GetName().c_str(), rig->GetName().c_str(),
	            weights.VertexCount(), weights.InfluenceCount());

	ImGui::Checkbox("Hide unused bones", &hideEmptyBones);
	ImGui::SameLine();
	ImGui::Checkbox("Blended vertices only", &blendedOnly);
	ImGui::SetItemTooltip("Only vertices split between more than one bone.");
	ImGui::SameLine();
	ImGui::BeginDisabled(!weldedAvailable);
	ImGui::Checkbox("Welded vertices", &welded);
	ImGui::EndDisabled();
	ImGui::SetItemTooltip(weldedAvailable
		? "One row per position: the copies a UV seam splits a vertex into share a row."
		: "The mesh has no topology to weld with.");

	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::InputInt("##jump", &jumpTarget, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		jumpRequested = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(welded ? "Go to welded vertex" : "Go to vertex"))
	{
		jumpRequested = true;
	}

	const CacheKey key{ mesh.get(), &weights, binding->revision, welded, hideEmptyBones, blendedOnly };
	if (!cacheValid || !(key == cacheKey))
	{
		Rebuild(key, weights, topology);
	}

	ImGui::SameLine();
	ImGui::TextDisabled("%zu rows", rows.size());

	DrawTable(skinning, weights, binding->influences);
}

void SkinWeightsSection::DrawTable(SkinningController& skinning, const SkinWeightTable& weights,
                                   const std::vector<SkinInfluence>& influences)
{
	// Parenthesised: windows.h, pulled in through the scene headers, defines min.
	const size_t shown = (std::min)(columns.size(), kMaxInfluenceColumns);
	if (shown < columns.size())
	{
		ImGui::TextColored(kWarningColor, "Showing the first %zu of %zu bones.", shown, columns.size());
	}

	const int columnCount = static_cast<int>(shown) + 2;
	const ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
		| ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV
		| ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;

	if (!ImGui::BeginTable("weights", columnCount, flags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
	{
		return;
	}

	ImGui::TableSetupScrollFreeze(1, 1);
	ImGui::TableSetupColumn("Vertex", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoHide,
	                        0.0f, kVertexColumnId);
	for (size_t c = 0; c < shown; ++c)
	{
		const size_t i = columns[c];
		ImGui::TableSetupColumn(influences[i].name.c_str(), ImGuiTableColumnFlags_WidthFixed,
		                        kWeightColumnWidth, static_cast<ImGuiID>(1 + i));
	}
	ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, 0.0f, kTotalColumnId);

	ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
	for (int col = 0; col < columnCount; ++col)
	{
		if (!ImGui::TableSetColumnIndex(col))
		{
			continue;
		}

		const char* name = ImGui::TableGetColumnName(col);
		const bool influenceColumn = col > 0 && col < columnCount - 1;
		if (influenceColumn)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, HeaderColor(influences[columns[col - 1]].color));
		}
		ImGui::TableHeader(name);
		if (influenceColumn)
		{
			ImGui::PopStyleColor();
			ImGui::SetItemTooltip("%s\nClick to sort. Click a weight below to show this bone in the viewport.", name);
		}
	}

	if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs(); specs && specs->SpecsDirty)
	{
		if (specs->SpecsCount > 0)
		{
			sortColumn = specs->Specs[0].ColumnUserID;
			sortDescending = specs->Specs[0].SortDirection == ImGuiSortDirection_Descending;
			Sort(weights);
		}
		specs->SpecsDirty = false;
	}

	int jumpIndex = -1;
	if (jumpRequested)
	{
		jumpRequested = false;
		const auto found = std::find(rows.begin(), rows.end(), jumpTarget);
		if (found != rows.end())
		{
			jumpIndex = static_cast<int>(found - rows.begin());
			selectedRow = jumpTarget;
		}
	}

	const int focusedBone = skinning.GetWeightView() == SkinWeightView::SingleInfluence
		? skinning.GetFocusedBone() : SkinningController::kNoBone;

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rows.size()));
	if (jumpIndex >= 0)
	{
		clipper.IncludeItemByIndex(jumpIndex);
	}

	while (clipper.Step())
	{
		for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
		{
			const int id = rows[r];
			const std::span<const float> row = weights.Row(SourceVertex(id));

			ImGui::TableNextRow();
			if (id == selectedRow)
			{
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_Header));
			}

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%d", id);
			if (r == jumpIndex)
			{
				ImGui::SetScrollHereY(0.5f);
			}

			ImGui::PushID(id);
			for (size_t c = 0; c < shown; ++c)
			{
				const size_t i = columns[c];
				const SkinInfluence& influence = influences[i];
				const float weight = row[i];
				ImGui::TableSetColumnIndex(static_cast<int>(c) + 1);

				if (weight > 0.0f)
				{
					const ImVec4 tint(influence.color.x, influence.color.y, influence.color.z,
					                  0.15f + 0.55f * weight);
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(tint));
				}

				char label[32];
				if (weight > 0.0f)
				{
					std::snprintf(label, sizeof(label), "%.3f##%zu", weight, i);
				}
				else
				{
					std::snprintf(label, sizeof(label), "-##%zu", i);
				}

				if (weight <= 0.0f)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
				}
				if (ImGui::Selectable(label, influence.bone == focusedBone && id == selectedRow))
				{
					selectedRow = id;
					skinning.SetFocusedBone(influence.bone);
					skinning.SetWeightView(SkinWeightView::SingleInfluence);
				}
				if (weight <= 0.0f)
				{
					ImGui::PopStyleColor();
				}
			}
			ImGui::PopID();

			ImGui::TableSetColumnIndex(columnCount - 1);
			const float total = RowTotal(row);
			if (std::fabs(total - 1.0f) > kTotalTolerance)
			{
				ImGui::TextColored(kWarningColor, "%.3f", total);
			}
			else
			{
				ImGui::Text("%.3f", total);
			}
		}
	}

	ImGui::EndTable();
}
