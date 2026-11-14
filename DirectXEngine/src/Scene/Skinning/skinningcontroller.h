#pragma once
#include "iskinningtechnique.h"
#include "skininfluence.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <wrl/client.h>

class Object3D;
class Skeleton;

class SkinningController
{
public:
	enum class Method
	{
		LinearBlend,
		DualQuaternion,
	};

	// How the weights feeding that technique are solved.
	enum class WeightMode
	{
		Default,
		BoneHeat,
	};

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

	void SetMethod(Method newMethod);
	Method GetMethod() const { return method; }
	const char* GetMethodName() const { return GetMethodName(method); }
	// Single source of the labels every panel shows.
	static const char* GetMethodName(Method method);

	void SetWeightMode(WeightMode newMode);
	WeightMode GetWeightMode() const { return weightMode; }
	const char* GetWeightModeName() const { return GetWeightModeName(weightMode); }
	static const char* GetWeightModeName(WeightMode mode);

	// Bone heat's alpha: how hard the solve is pulled back to the rigid binding.
	void SetBoneHeatAlpha(float newAlpha);
	float GetBoneHeatAlpha() const { return boneHeatAlpha; }

	// Re-solves the weights of every bound mesh in the bind pose it was captured
	// in. Needed after the weight strategy, or anything feeding it, changes.
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

	// Read access for the weight table. The references stay valid until the
	// next bind, unbind or re-solve, so take a fresh view every frame.
	struct BindingView
	{
		const SkinWeightTable& weights;
		const std::vector<SkinInfluence>& influences;
		// Changes whenever the weights are re-solved, so derived data can be cached.
		uint64_t revision;
	};
	std::optional<BindingView> GetBinding(const Object3D& mesh) const;

	// Re-solves the weights of every bound mesh. Needed after the rig gains a
	// joint or a rest offset moves, because the bind pose is then a different one.
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
		DirectX::XMFLOAT4X4 meshBindToRig;
		// Restored when the weight view goes off or the mesh is unbound.
		std::wstring originalVertexShader;
		std::wstring originalPixelShader;
		std::unique_ptr<ISkinningTechnique> technique;
		uint64_t revision = 0;
	};

	// Points sampled along one influence's bone, in the rig's bind pose.
	struct BoneSamples
	{
		std::vector<DirectX::XMFLOAT3> points;
	};

	std::unique_ptr<ISkinningTechnique> CreateTechnique() const;
	// One influence per bone, or a single root one for a rig without bones.
	std::vector<SkinInfluence> BuildInfluences() const;
	// One entry per influence, in the same order.
	std::vector<BoneSamples> SampleBones(const std::vector<SkinInfluence>& influences) const;
	// Closest bone takes the whole vertex, as in the reference implementation.
	// closestDistance comes back holding each vertex's distance to that bone,
	// which is what bone heat weighs its constraint by.
	SkinWeightTable CalculateWeights(const std::vector<SkinInfluence>& influences,
	                                 const std::vector<VertexData>& restVertices,
	                                 DirectX::FXMMATRIX meshBindToRig,
	                                 std::vector<float>& closestDistance) const;
	bool Solve(BoundMesh& bound) const;
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
	Method method = Method::LinearBlend;
	WeightMode weightMode = WeightMode::Default;
	// The epsilon the reference implementation defaults to.
	float boneHeatAlpha = 0.001f;
	SkinWeightView weightView = SkinWeightView::Off;
	int focusedBone = kNoBone;
	// Source of BoundMesh::revision; Solve is const but hands out new ones.
	mutable uint64_t lastRevision = 0;
};
