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
		std::vector<SkinWeights> weights;
		DirectX::XMFLOAT4X4 meshBindToRig;
		size_t jointCount = 0;
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
	std::vector<SkinWeights> CalculateWeights(const std::vector<VertexData>& restVertices,
	                                          DirectX::FXMMATRIX meshBindToRig) const;
	bool Solve(BoundMesh& bound) const;
	void ApplyWeightView(const BoundMesh& bound, Object3D& mesh) const;
	void BuildPalette(const BoundMesh& bound, const Object3D& mesh,
	                  std::vector<DirectX::XMFLOAT4X4>& palette) const;

	Skeleton& rig;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	std::vector<BoundMesh> boundMeshes;
	Method method = Method::LinearBlend;
	bool showWeights = false;
};
