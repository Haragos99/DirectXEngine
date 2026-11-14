#include "skinningcontroller.h"

#include "boneheat.h"
#include "dualquaternionskinning.h"
#include "halfedgemesh.h"
#include "linearblendskinning.h"
#include "object3d.h"
#include "skeleton.h"

#include <cmath>
#include <limits>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
	// Enough points that a long bone still wins over a short one nearby.
	constexpr int kSamplesPerBone = 16;

	const wchar_t* kWeightVertexShader = L"shaders\\SkinWeightVertexShader.hlsl";
	const wchar_t* kWeightPixelShader = L"shaders\\SkinWeightPixelShader.hlsl";
}

SkinningController::SkinningController(Skeleton& _rig, ComPtr<ID3D11Device> _device, ComPtr<ID3D11DeviceContext> _context)
	: rig(_rig), device(_device), context(_context)
{
}

SkinningController::~SkinningController()
{
	UnbindAll();
}

std::unique_ptr<ISkinningTechnique> SkinningController::CreateTechnique() const
{
	switch (method)
	{
	case Method::DualQuaternion:
		return std::make_unique<DualQuaternionSkinning>(device, context);
	case Method::LinearBlend:
	default:
		return std::make_unique<LinearBlendSkinning>(device, context);
	}
}

const char* SkinningController::GetMethodName(Method method)
{
	switch (method)
	{
	case Method::DualQuaternion:
		return "Dual Quaternion (DQS)";
	case Method::LinearBlend:
	default:
		return "Linear Blend (LBS)";
	}
}

const char* SkinningController::GetWeightModeName(WeightMode mode)
{
	switch (mode)
	{
	case WeightMode::BoneHeat:
		return "Bone Heat";
	case WeightMode::Default:
	default:
		return "Default (closest bone)";
	}
}

const char* SkinningController::GetWeightViewName(SkinWeightView view)
{
	switch (view)
	{
	case SkinWeightView::Blended:
		return "Blended bone colours";
	case SkinWeightView::SingleInfluence:
		return "Single bone (heat)";
	case SkinWeightView::Off:
	default:
		return "Off";
	}
}

std::vector<XMFLOAT3> SkinningController::InfluenceColors(const BoundMesh& bound)
{
	std::vector<XMFLOAT3> colors;
	colors.reserve(bound.influences.size());
	for (const SkinInfluence& influence : bound.influences)
		colors.push_back(influence.color);
	return colors;
}

void SkinningController::RestoreShaders(const BoundMesh& bound, Object3D& mesh)
{
	mesh.SetShaders(bound.originalVertexShader, bound.originalPixelShader);
}

void SkinningController::ApplyWeightView(const BoundMesh& bound, Object3D& mesh) const
{
	// The focused bone is a rig-wide choice; each mesh has its own columns.
	int selected = -1;
	for (size_t i = 0; i < bound.influences.size(); ++i)
	{
		if (bound.influences[i].bone == focusedBone)
			selected = static_cast<int>(i);
	}

	bound.technique->SetWeightView(weightView, selected);
	if (weightView == SkinWeightView::Off)
		RestoreShaders(bound, mesh);
	else
		mesh.SetShaders(kWeightVertexShader, kWeightPixelShader);
}

void SkinningController::ApplyWeightViewToAll() const
{
	for (const BoundMesh& bound : boundMeshes)
	{
		if (const std::shared_ptr<Object3D> mesh = bound.mesh.lock())
			ApplyWeightView(bound, *mesh);
	}
}

void SkinningController::SetWeightView(SkinWeightView view)
{
	if (view == weightView)
		return;

	weightView = view;
	ApplyWeightViewToAll();
}

void SkinningController::SetFocusedBone(int bone)
{
	if (bone == focusedBone)
		return;

	focusedBone = bone;
	ApplyWeightViewToAll();
}

int SkinningController::GetHighlightedBone() const
{
	return weightView == SkinWeightView::SingleInfluence && focusedBone >= 0 ? focusedBone : -1;
}

const std::vector<SkinInfluence>* SkinningController::GetInfluences(const Object3D& mesh) const
{
	for (const BoundMesh& bound : boundMeshes)
	{
		if (bound.mesh.lock().get() == &mesh)
			return &bound.influences;
	}
	return nullptr;
}

void SkinningController::SetMethod(Method newMethod)
{
	if (newMethod == method)
	{
		return;
	}

	method = newMethod;
	for (BoundMesh& bound : boundMeshes)
	{
		bound.technique = CreateTechnique();
		bound.technique->Prepare(bound.restVertices, bound.weights, InfluenceColors(bound));

		if (const std::shared_ptr<Object3D> mesh = bound.mesh.lock())
		{
			mesh->SetDeformedVertices(bound.technique->GetDeformedVertices());
			ApplyWeightView(bound, *mesh);
		}
	}
}

void SkinningController::SetWeightMode(WeightMode newMode)
{
	if (newMode == weightMode)
	{
		return;
	}

	weightMode = newMode;
	ResolveWeights();
}

void SkinningController::SetBoneHeatAlpha(float newAlpha)
{
	if (newAlpha == boneHeatAlpha)
	{
		return;
	}

	boneHeatAlpha = newAlpha;
	if (weightMode == WeightMode::BoneHeat)
	{
		ResolveWeights();
	}
}

void SkinningController::ResolveWeights()
{
	for (BoundMesh& bound : boundMeshes)
	{
		const std::shared_ptr<Object3D> mesh = bound.mesh.lock();
		if (!mesh || !Solve(bound))
			continue;

		mesh->SetDeformedVertices(bound.technique->GetDeformedVertices());
		// The columns may have changed, and with them the focused one's index.
		ApplyWeightView(bound, *mesh);
	}
}

std::vector<SkinInfluence> SkinningController::BuildInfluences() const
{
	const int jointCount = static_cast<int>(rig.GetJointCount());

	std::vector<SkinInfluence> influences;
	for (size_t b = 0; b < rig.GetBones().size(); ++b)
	{
		const Bone& bone = rig.GetBones()[b];
		const int start = bone.GetStartJoint();
		const int end = bone.GetEndJoint();
		if (start < 0 || start >= jointCount || end < 0 || end >= jointCount)
		{
			continue;
		}

		SkinInfluence influence;
		influence.bone = static_cast<int>(b);
		// The bone start->end sits still in the start joint's frame, so that
		// joint's skinning matrix is what carries the skin around it.
		influence.drivingJoint = start;
		influence.color = bone.GetColor();
		influence.name = rig.GetJoint(start).GetName() + " > " + rig.GetJoint(end).GetName();
		influences.push_back(std::move(influence));
	}

	// Without bones every vertex simply rides on the root.
	if (influences.empty() && jointCount > 0)
	{
		SkinInfluence root;
		root.drivingJoint = 0;
		root.color = XMFLOAT3(0.6f, 0.6f, 0.6f);
		root.name = rig.GetJoint(0).GetName();
		influences.push_back(std::move(root));
	}
	return influences;
}

std::vector<SkinningController::BoneSamples> SkinningController::SampleBones(const std::vector<SkinInfluence>& influences) const
{
	const std::vector<XMFLOAT4X4>& bindGlobals = rig.GetBindGlobals();
	const auto bindPosition = [&](int joint)
	{
		return joint >= 0 && joint < static_cast<int>(bindGlobals.size())
			? XMLoadFloat4x4(&bindGlobals[joint]).r[3]
			: XMVectorZero();
	};

	std::vector<BoneSamples> samples(influences.size());
	for (size_t i = 0; i < influences.size(); ++i)
	{
		const SkinInfluence& influence = influences[i];
		if (influence.bone == SkinInfluence::kRootFallback)
		{
			XMFLOAT3 point;
			XMStoreFloat3(&point, bindPosition(influence.drivingJoint));
			samples[i].points.push_back(point);
			continue;
		}

		const Bone& bone = rig.GetBones()[influence.bone];
		const XMVECTOR from = bindPosition(bone.GetStartJoint());
		const XMVECTOR to = bindPosition(bone.GetEndJoint());

		samples[i].points.reserve(kSamplesPerBone + 1);
		for (int s = 0; s <= kSamplesPerBone; ++s)
		{
			XMFLOAT3 point;
			XMStoreFloat3(&point, XMVectorLerp(from, to, static_cast<float>(s) / kSamplesPerBone));
			samples[i].points.push_back(point);
		}
	}
	return samples;
}

SkinWeightTable SkinningController::CalculateWeights(const std::vector<SkinInfluence>& influences,
                                                     const std::vector<VertexData>& restVertices,
                                                     FXMMATRIX meshBindToRig,
                                                     std::vector<float>& closestDistance) const
{
	const std::vector<BoneSamples> bones = SampleBones(influences);

	SkinWeightTable weights;
	weights.Resize(restVertices.size(), influences.size());
	closestDistance.assign(restVertices.size(), 0.0f);

	for (size_t v = 0; v < restVertices.size(); ++v)
	{
		const XMVECTOR position = XMVector3Transform(XMLoadFloat3(&restVertices[v].position), meshBindToRig);

		size_t closestInfluence = 0;
		float closest = std::numeric_limits<float>::infinity();
		for (size_t i = 0; i < bones.size(); ++i)
		{
			for (const XMFLOAT3& point : bones[i].points)
			{
				const float distance = XMVectorGetX(
					XMVector3LengthSq(XMVectorSubtract(XMLoadFloat3(&point), position)));
				if (distance < closest)
				{
					closest = distance;
					closestInfluence = i;
				}
			}
		}

		if (closestInfluence < weights.InfluenceCount())
		{
			weights[v][closestInfluence] = 1.0f;
		}
		if (std::isfinite(closest))
		{
			closestDistance[v] = std::sqrt(closest);
		}
	}
	return weights;
}

bool SkinningController::Solve(BoundMesh& bound) const
{
	bound.influences = BuildInfluences();
	if (bound.influences.empty() || bound.restVertices.empty())
	{
		return false;
	}

	std::vector<float> closestDistance;
	bound.weights = CalculateWeights(bound.influences, bound.restVertices, XMLoadFloat4x4(&bound.meshBindToRig), closestDistance);

	if (weightMode == WeightMode::BoneHeat)
	{
		const std::shared_ptr<Object3D> mesh = bound.mesh.lock();
		const HalfEdgeMesh* topology = mesh ? mesh->GetHalfEdgeMesh() : nullptr;
		// Without connectivity there is no surface to diffuse over, so the rigid
		// binding is what the technique gets.
		if (topology != nullptr && !topology->IsEmpty())
		{
			BoneHeat(boneHeatAlpha).Execute(*topology, closestDistance, bound.weights);
		}
	}

	return bound.technique->Prepare(bound.restVertices, bound.weights, InfluenceColors(bound));
}

bool SkinningController::BindMesh(const std::shared_ptr<Object3D>& mesh)
{
	if (!mesh || IsBound(*mesh))
	{
		return false;
	}

	const std::vector<VertexData>* source = mesh->GetSkinVertices();
	if (source == nullptr || source->empty())
	{
		return false;
	}

	BoundMesh bound;
	bound.mesh = mesh;
	bound.restVertices = *source;
	bound.originalVertexShader = mesh->GetVertexShaderPath();
	bound.originalPixelShader = mesh->GetPixelShaderPath();
	bound.technique = CreateTechnique();

	// Where the mesh sits inside the rig at the moment of binding: everything
	// the rig does later is measured against this.
	XMStoreFloat4x4(&bound.meshBindToRig,mesh->GetWorldMatrix() * XMMatrixInverse(nullptr, rig.GetWorldMatrix()));

	if (!Solve(bound))
	{
		return false;
	}

	mesh->SetDeformedVertices(bound.technique->GetDeformedVertices());
	ApplyWeightView(bound, *mesh);
	boundMeshes.push_back(std::move(bound));
	return true;
}

void SkinningController::UnbindMesh(const Object3D& mesh)
{
	for (auto it = boundMeshes.begin(); it != boundMeshes.end(); ++it)
	{
		const std::shared_ptr<Object3D> bound = it->mesh.lock();
		if (bound.get() != &mesh)
		{
			continue;
		}

		bound->SetDeformedVertices(nullptr);
		RestoreShaders(*it, *bound);
		boundMeshes.erase(it);
		return;
	}
}

void SkinningController::UnbindAll()
{
	for (BoundMesh& bound : boundMeshes)
	{
		if (const std::shared_ptr<Object3D> mesh = bound.mesh.lock())
		{
			mesh->SetDeformedVertices(nullptr);
			RestoreShaders(bound, *mesh);
		}
	}
	boundMeshes.clear();
}

bool SkinningController::IsBound(const Object3D& mesh) const
{
	for (const BoundMesh& bound : boundMeshes)
	{
		if (bound.mesh.lock().get() == &mesh)
			return true;
	}
	return false;
}

void SkinningController::RebindAll()
{
	for (BoundMesh& bound : boundMeshes)
	{
		const std::shared_ptr<Object3D> mesh = bound.mesh.lock();
		if (!mesh)
			continue;

		XMStoreFloat4x4(&bound.meshBindToRig,
			mesh->GetWorldMatrix() * XMMatrixInverse(nullptr, rig.GetWorldMatrix()));
		Solve(bound);
		mesh->SetDeformedVertices(bound.technique->GetDeformedVertices());
		ApplyWeightView(bound, *mesh);
	}
}

void SkinningController::BuildPalette(const BoundMesh& bound, const Object3D& mesh, std::vector<XMFLOAT4X4>& palette) const
{
	const std::vector<XMFLOAT4X4>& poseMatrices = rig.GetSkinningMatrices();
	const XMMATRIX meshBindToRig = XMLoadFloat4x4(&bound.meshBindToRig);
	// Back out of the mesh's own world matrix, which the vertex shader reapplies.
	const XMMATRIX rigToMesh = rig.GetWorldMatrix() * XMMatrixInverse(nullptr, mesh.GetWorldMatrix());

	palette.resize(bound.influences.size());
	for (size_t i = 0; i < bound.influences.size(); ++i)
	{
		const int joint = bound.influences[i].drivingJoint;
		const XMMATRIX skinning = joint >= 0 && joint < static_cast<int>(poseMatrices.size())
			? XMLoadFloat4x4(&poseMatrices[joint])
			: XMMatrixIdentity();
		XMStoreFloat4x4(&palette[i], meshBindToRig * skinning * rigToMesh);
	}
}

void SkinningController::Update()
{
	std::vector<XMFLOAT4X4> palette;
	for (auto it = boundMeshes.begin(); it != boundMeshes.end();)
	{
		const std::shared_ptr<Object3D> mesh = it->mesh.lock();
		if (!mesh)
		{
			it = boundMeshes.erase(it);
			continue;
		}

		BuildPalette(*it, *mesh, palette);
		it->technique->Deform(palette);
		++it;
	}
}
