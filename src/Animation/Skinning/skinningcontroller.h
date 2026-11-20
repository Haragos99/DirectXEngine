#pragma once
#include "iskindeformer.h"
#include "iweightsolver.h"
#include "skinbindcontext.h"
#include "skininfluence.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <wrl/client.h>

class Object3D;
class Skeleton;

class SkinningController
{
public:
	SkinningController(Skeleton& _rig,
	                   Microsoft::WRL::ComPtr<ID3D11Device> _device,
	                   Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
	~SkinningController();

	// Captures the mesh where it currently stands and starts deforming it.
	bool BindMesh(const std::shared_ptr<Object3D>& mesh);
	void UnbindMesh(const Object3D& mesh);
	void UnbindAll();
	bool IsBound(const Object3D& mesh) const;
	size_t GetBoundMeshCount() const { return boundMeshes.size(); }

	void SetDeformer(size_t index);
	size_t GetDeformerIndex() const { return deformerIndex; }
	const char* GetDeformerName() const;

	void SetWeightSolver(size_t index);
	size_t GetWeightSolverIndex() const { return solverIndex; }
	const char* GetWeightSolverName() const;
	std::span<const SolverParameter> GetSolverParameters() const;
	void SetSolverParameter(size_t parameter, float value);

	void ResolveWeights();

	void SetWeightView(SkinWeightView view);
	SkinWeightView GetWeightView() const { return weightView; }
	static const char* GetWeightViewName(SkinWeightView view);

	static constexpr int kNoBone = -2;
	void SetFocusedBone(int bone);
	int GetFocusedBone() const { return focusedBone; }
	int GetHighlightedBone() const;

	const std::vector<SkinInfluence>* GetInfluences(const Object3D& mesh) const;

	struct BindingView
	{
		const SkinWeightTable& weights;
		const std::vector<SkinInfluence>& influences;
		WeightSolveStatus status;
		uint64_t revision;
	};
	std::optional<BindingView> GetBinding(const Object3D& mesh) const;

	void RebindAll();

	// Re-deforms every bound mesh from the rig's current pose.
	void Update();

private:
	struct BoundMesh
	{
		std::weak_ptr<Object3D> mesh;
		std::vector<VertexData> restVertices; // mesh local space, as bound
		std::vector<SkinInfluence> influences;
		SkinWeightTable weights;
		WeightSolveStatus status = WeightSolveStatus::Ok;
		uint64_t revision = 0;
		DirectX::XMFLOAT4X4 meshBindToRig;
		std::wstring originalVertexShader;
		std::wstring originalPixelShader;
		std::unique_ptr<ISkinDeformer> deformer;
	};

	enum class RebuildScope
	{
		Deformer,
		Weights,
		BindPose,
	};

	bool Rebuild(BoundMesh& bound, Object3D& mesh, RebuildScope scope);
	void RebuildAll(RebuildScope scope);

	std::unique_ptr<ISkinDeformer> CreateDeformer() const;
	std::vector<SkinInfluence> BuildInfluences() const;
	SkinBindContext BuildBindContext(const BoundMesh& bound, const Object3D& mesh) const;
	void SolveWeights(BoundMesh& bound, const Object3D& mesh);

	static std::vector<DirectX::XMFLOAT3> InfluenceColors(const BoundMesh& bound);
	static void RestoreShaders(const BoundMesh& bound, Object3D& mesh);
	void ApplyWeightViewToAll() const;
	void ApplyWeightView(const BoundMesh& bound, Object3D& mesh) const;
	void BuildPalette(const BoundMesh& bound, const Object3D& mesh,
	                  std::vector<DirectX::XMFLOAT4X4>& palette) const;

	Skeleton& rig;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	std::vector<BoundMesh> boundMeshes;

	size_t deformerIndex = 0;
	size_t solverIndex = 0;
	std::vector<std::unique_ptr<IWeightSolver>> solvers;

	SkinWeightView weightView = SkinWeightView::Off;
	int focusedBone = kNoBone;
	uint64_t lastRevision = 0;
};
