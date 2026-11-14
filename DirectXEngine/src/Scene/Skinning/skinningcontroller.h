#pragma once
#include "iskinningtechnique.h"

#include <memory>
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

	// Draws every bound mesh coloured by the joint that drives it.
	void SetShowWeights(bool show);
	bool GetShowWeights() const { return showWeights; }

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
		SkinWeightTable weights;
		DirectX::XMFLOAT4X4 meshBindToRig;
		std::wstring originalPixelShader; // restored when the weight view goes off
		std::unique_ptr<ISkinningTechnique> technique;
	};

	// Points sampled along one bone, in the rig's bind pose.
	struct BoneSamples
	{
		int jointId = 0;
		std::vector<DirectX::XMFLOAT3> points;
	};

	std::unique_ptr<ISkinningTechnique> CreateTechnique() const;
	std::vector<BoneSamples> SampleBones() const;
	// Closest bone takes the whole vertex, as in the reference implementation.
	// closestDistance comes back holding each vertex's distance to that bone,
	// which is what bone heat weighs its constraint by.
	SkinWeightTable CalculateWeights(const std::vector<VertexData>& restVertices,
	                                 DirectX::FXMMATRIX meshBindToRig,
	                                 std::vector<float>& closestDistance) const;
	bool Solve(BoundMesh& bound) const;
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
	bool showWeights = false;
};
