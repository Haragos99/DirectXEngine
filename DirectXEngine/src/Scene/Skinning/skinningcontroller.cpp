#include "skinningcontroller.h"

#include "linearblendskinning.h"
#include "object3d.h"
#include "skeleton.h"

#include <limits>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
	// Enough points that a long bone still wins over a short one nearby.
	constexpr int kSamplesPerBone = 16;

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
	case Method::LinearBlend:
	default:
		return std::make_unique<LinearBlendSkinning>(device, context);
	}
}

const char* SkinningController::GetMethodName() const
{
	switch (method)
	{
	case Method::LinearBlend:
	default:
		return "Linear Blend";
	}
}

void SkinningController::ApplyWeightView(const BoundMesh& bound, Object3D& mesh) const
{
	bound.technique->SetWeightDebug(showWeights);
	mesh.SetPixelShader(showWeights ? kWeightPixelShader : bound.originalPixelShader);
}

void SkinningController::SetShowWeights(bool show)
{
	if (show == showWeights)
		return;

	showWeights = show;
	for (const BoundMesh& bound : boundMeshes)
	{
		if (const std::shared_ptr<Object3D> mesh = bound.mesh.lock())
			ApplyWeightView(bound, *mesh);
	}
}

void SkinningController::SetMethod(Method newMethod)
{
	if (newMethod == method)
		return;

	method = newMethod;
	for (BoundMesh& bound : boundMeshes)
	{
		bound.technique = CreateTechnique();
		bound.technique->Prepare(bound.restVertices, bound.weights, static_cast<int>(bound.jointCount));

		if (const std::shared_ptr<Object3D> mesh = bound.mesh.lock())
		{
			mesh->SetDeformedVertices(bound.technique->GetDeformedVertices());
			ApplyWeightView(bound, *mesh);
		}
	}
}

std::vector<SkinningController::BoneSamples> SkinningController::SampleBones() const
{
	const std::vector<XMFLOAT4X4>& bindGlobals = rig.GetBindGlobals();

	std::vector<BoneSamples> samples;
	for (const Bone& bone : rig.GetBones())
	{
		const int start = bone.GetStartJoint();
		const int end = bone.GetEndJoint();
		if (start >= static_cast<int>(bindGlobals.size()) || end >= static_cast<int>(bindGlobals.size()))
			continue;

		const XMVECTOR from = XMLoadFloat4x4(&bindGlobals[start]).r[3];
		const XMVECTOR to = XMLoadFloat4x4(&bindGlobals[end]).r[3];

		BoneSamples boneSamples;
		boneSamples.jointId = end; // the end joint is what drives this bone
		boneSamples.points.reserve(kSamplesPerBone + 1);
		for (int i = 0; i <= kSamplesPerBone; ++i)
		{
			XMFLOAT3 point;
			XMStoreFloat3(&point, XMVectorLerp(from, to, static_cast<float>(i) / kSamplesPerBone));
			boneSamples.points.push_back(point);
		}
		samples.push_back(std::move(boneSamples));
	}
	return samples;
}

std::vector<SkinWeights> SkinningController::CalculateWeights(const std::vector<VertexData>& restVertices,
                                                    FXMMATRIX meshBindToRig) const
{
	const std::vector<BoneSamples> bones = SampleBones();

	std::vector<SkinWeights> weights(restVertices.size());
	for (size_t v = 0; v < restVertices.size(); ++v)
	{
		const XMVECTOR position = XMVector3Transform(XMLoadFloat3(&restVertices[v].position), meshBindToRig);

		int closestJoint = 0;
		float closest = std::numeric_limits<float>::infinity();
		for (const BoneSamples& bone : bones)
		{
			for (const XMFLOAT3& point : bone.points)
			{
				const float distance = XMVectorGetX(
					XMVector3LengthSq(XMVectorSubtract(XMLoadFloat3(&point), position)));
				if (distance < closest)
				{
					closest = distance;
					closestJoint = bone.jointId;
				}
			}
		}

		weights[v].joints[0] = static_cast<uint32_t>(closestJoint);
		weights[v].weights[0] = 1.0f;
	}
	return weights;
}

bool SkinningController::Solve(BoundMesh& bound) const
{
	bound.jointCount = rig.GetJointCount();
	if (bound.jointCount == 0 || bound.restVertices.empty())
		return false;

	bound.weights = CalculateWeights(bound.restVertices, XMLoadFloat4x4(&bound.meshBindToRig));
	return bound.technique->Prepare(bound.restVertices, bound.weights, static_cast<int>(bound.jointCount));
}

bool SkinningController::BindMesh(const std::shared_ptr<Object3D>& mesh)
{
	if (!mesh || IsBound(*mesh))
		return false;

	const std::vector<VertexData>* source = mesh->GetSkinVertices();
	if (source == nullptr || source->empty())
		return false;

	BoundMesh bound;
	bound.mesh = mesh;
	bound.restVertices = *source;
	bound.originalPixelShader = mesh->GetPixelShaderPath();
	bound.technique = CreateTechnique();

	// Where the mesh sits inside the rig at the moment of binding: everything
	// the rig does later is measured against this.
	XMStoreFloat4x4(&bound.meshBindToRig,
		mesh->GetWorldMatrix() * XMMatrixInverse(nullptr, rig.GetWorldMatrix()));

	if (!Solve(bound))
		return false;

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
			continue;

		bound->SetDeformedVertices(nullptr);
		bound->SetPixelShader(it->originalPixelShader);
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
			mesh->SetPixelShader(bound.originalPixelShader);
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
	}
}

void SkinningController::BuildPalette(const BoundMesh& bound, const Object3D& mesh,
                            std::vector<XMFLOAT4X4>& palette) const
{
	const std::vector<XMFLOAT4X4>& poseMatrices = rig.GetSkinningMatrices();
	const XMMATRIX meshBindToRig = XMLoadFloat4x4(&bound.meshBindToRig);
	// Back out of the mesh's own world matrix, which the vertex shader reapplies.
	const XMMATRIX rigToMesh = rig.GetWorldMatrix() * XMMatrixInverse(nullptr, mesh.GetWorldMatrix());

	palette.resize(poseMatrices.size());
	for (size_t i = 0; i < poseMatrices.size(); ++i)
		XMStoreFloat4x4(&palette[i], meshBindToRig * XMLoadFloat4x4(&poseMatrices[i]) * rigToMesh);
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
