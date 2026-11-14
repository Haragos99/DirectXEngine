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

// Binds meshes to one skeleton and keeps them deformed. Owns the binding
// lifecycle only; the techniques are pluggable stages picked from
// SkinningRegistry:
//
//   bind time: IWeightSolver  -> weights
//   per frame: ISkinDeformer  -> deformed vertex buffer   (-> ISkinCorrective, later)
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

	// Deformer, as an index into SkinningRegistry::Deformers().
	void SetDeformer(size_t index);
	size_t GetDeformerIndex() const { return deformerIndex; }
	const char* GetDeformerName() const;

	// Weight solver, as an index into SkinningRegistry::WeightSolvers(). Each
	// solver keeps its own parameters when another one is picked in between.
	void SetWeightSolver(size_t index);
	size_t GetWeightSolverIndex() const { return solverIndex; }
	const char* GetWeightSolverName() const;
	std::span<const SolverParameter> GetSolverParameters() const;
	// Clamps to the parameter's range and re-solves every bound mesh.
	void SetSolverParameter(size_t parameter, float value);

	// Re-solves the weights of every bound mesh in the bind pose it was captured in.
	void ResolveWeights();

	// Draws every bound mesh coloured by its weights instead of its own shading:
	// blended from the bones' colours, or as a heat ramp of the focused bone.
	void SetWeightView(SkinWeightView view);
	SkinWeightView GetWeightView() const { return weightView; }
	static const char* GetWeightViewName(SkinWeightView view);

	// The bone the single-influence view shows: an index into the rig's bones,
	// SkinInfluence::kRootFallback for a rig without bones, or kNoBone.
	static constexpr int kNoBone = -2;
	void SetFocusedBone(int bone);
	int GetFocusedBone() const { return focusedBone; }
	// The bone the skeleton should draw highlighted, -1 for none.
	int GetHighlightedBone() const;

	// Columns of the bound mesh's weight table, null when it is not bound here.
	const std::vector<SkinInfluence>* GetInfluences(const Object3D& mesh) const;

	// Read access for the UI. The references stay valid until the next bind,
	// unbind or re-solve, so take a fresh view every frame.
	struct BindingView
	{
		const SkinWeightTable& weights;
		const std::vector<SkinInfluence>& influences;
		// Anything but Ok means the solver fell back to the closest-bone weights.
		WeightSolveStatus status;
		// Changes whenever the weights are re-solved, so derived data can be cached.
		uint64_t revision;
	};
	std::optional<BindingView> GetBinding(const Object3D& mesh) const;

	// Recaptures where every bound mesh sits in the rig and re-solves it. Needed
	// after the rig gains a joint or a rest offset moves.
	void RebindAll();

	// Re-deforms every bound mesh from the rig's current pose.
	void Update();

private:
	struct BoundMesh
	{
		std::weak_ptr<Object3D> mesh;
		std::vector<VertexData> restVertices; // mesh local space, as bound
		std::vector<SkinInfluence> influences; // the columns of `weights`
		SkinWeightTable weights;
		WeightSolveStatus status = WeightSolveStatus::Ok;
		uint64_t revision = 0;
		DirectX::XMFLOAT4X4 meshBindToRig;
		// Restored when the weight view goes off or the mesh is unbound.
		std::wstring originalVertexShader;
		std::wstring originalPixelShader;
		std::unique_ptr<ISkinDeformer> deformer;
	};

	// How much of a binding has to be redone; each scope includes the ones below.
	enum class RebuildScope
	{
		Deformer, // new deformer, same weights
		Weights,  // re-solve, same bind transform
		BindPose, // recapture where the mesh sits in the rig, then re-solve
	};

	// The one place a binding is (re)built and handed to the mesh. False leaves
	// the mesh drawing its own vertices with its own shaders.
	bool Rebuild(BoundMesh& bound, Object3D& mesh, RebuildScope scope);
	void RebuildAll(RebuildScope scope);

	std::unique_ptr<ISkinDeformer> CreateDeformer() const;
	// One influence per bone, or a single root one for a rig without bones.
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
	// One instance per registry entry, so parameters survive switching.
	std::vector<std::unique_ptr<IWeightSolver>> solvers;

	SkinWeightView weightView = SkinWeightView::Off;
	int focusedBone = kNoBone;
	uint64_t lastRevision = 0; // source of BoundMesh::revision
};
