#include "skinningcontroller.h"

#include "object3d.h"
#include "skeleton.h"
#include "skinningregistry.h"

#include <algorithm>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
	const wchar_t* kWeightVertexShader = L"shaders\\SkinWeightVertexShader.hlsl";
	const wchar_t* kWeightPixelShader = L"shaders\\SkinWeightPixelShader.hlsl";
}

SkinningController::SkinningController(Skeleton& _rig, ComPtr<ID3D11Device> _device, ComPtr<ID3D11DeviceContext> _context)
	: rig(_rig), device(_device), context(_context)
{
	for (const WeightSolverEntry& entry : SkinningRegistry::WeightSolvers())
		solvers.push_back(entry.create());
}

SkinningController::~SkinningController()
{
	UnbindAll();
}

// ---------------------------------------------------------------- techniques

std::unique_ptr<ISkinDeformer> SkinningController::CreateDeformer() const
{
	return SkinningRegistry::Deformers()[deformerIndex].create(device, context);
}

const char* SkinningController::GetDeformerName() const
{
	return SkinningRegistry::Deformers()[deformerIndex].name;
}

void SkinningController::SetDeformer(size_t index)
{
	if (index == deformerIndex || index >= SkinningRegistry::Deformers().size())
		return;

	deformerIndex = index;
	RebuildAll(RebuildScope::Deformer);
}

const char* SkinningController::GetWeightSolverName() const
{
	return SkinningRegistry::WeightSolvers()[solverIndex].name;
}

void SkinningController::SetWeightSolver(size_t index)
{
	if (index == solverIndex || index >= solvers.size())
		return;

	solverIndex = index;
	ResolveWeights();
}

std::span<const SolverParameter> SkinningController::GetSolverParameters() const
{
	const IWeightSolver& solver = *solvers[solverIndex];
	return solver.GetParameters();
}

void SkinningController::SetSolverParameter(size_t parameter, float value)
{
	const std::span<SolverParameter> parameters = solvers[solverIndex]->GetParameters();
	if (parameter >= parameters.size())
		return;

	// The limits live with the solver, so a typed-in value cannot break it.
	SolverParameter& target = parameters[parameter];
	value = std::clamp(value, target.min, target.max);
	if (value == target.value)
		return;

	target.value = value;
	ResolveWeights();
}

void SkinningController::ResolveWeights()
{
	RebuildAll(RebuildScope::Weights);
}

// ---------------------------------------------------------------- weight view

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

	bound.deformer->SetWeightView(weightView, selected);
	if (weightView == SkinWeightView::Off)
		RestoreShaders(bound, mesh);
	else
		mesh.SetShaders(kWeightVertexShader, kWeightPixelShader);
}

void SkinningController::ApplyWeightViewToAll() const
{
	for (const BoundMesh& bound : boundMeshes)
	{
		const std::shared_ptr<Object3D> mesh = bound.mesh.lock();
		if (mesh && bound.deformer && bound.deformer->GetDeformedVertices())
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

// ---------------------------------------------------------------- read access

std::optional<SkinningController::BindingView> SkinningController::GetBinding(const Object3D& mesh) const
{
	for (const BoundMesh& bound : boundMeshes)
	{
		if (bound.mesh.lock().get() == &mesh)
			return BindingView{ bound.weights, bound.influences, bound.status, bound.revision };
	}
	return std::nullopt;
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

// ---------------------------------------------------------------- solving

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

SkinBindContext SkinningController::BuildBindContext(const BoundMesh& bound, const Object3D& mesh) const
{
	const std::vector<XMFLOAT4X4>& bindGlobals = rig.GetBindGlobals();
	const auto bindPosition = [&](int joint)
	{
		if (joint < 0 || joint >= static_cast<int>(bindGlobals.size()))
			return XMFLOAT3(0.0f, 0.0f, 0.0f);
		const XMFLOAT4X4& m = bindGlobals[joint];
		return XMFLOAT3(m._41, m._42, m._43);
	};

	std::vector<BoneSegment> bones;
	bones.reserve(bound.influences.size());
	for (const SkinInfluence& influence : bound.influences)
	{
		if (influence.bone == SkinInfluence::kRootFallback)
		{
			const XMFLOAT3 root = bindPosition(influence.drivingJoint);
			bones.push_back({ root, root });
			continue;
		}

		const Bone& bone = rig.GetBones()[influence.bone];
		bones.push_back({ bindPosition(bone.GetStartJoint()), bindPosition(bone.GetEndJoint()) });
	}

	return SkinBindContext{ bound.restVertices, mesh.GetHalfEdgeMesh(), bound.meshBindToRig, std::move(bones) };
}

void SkinningController::SolveWeights(BoundMesh& bound, const Object3D& mesh)
{
	bound.influences = BuildInfluences();
	bound.revision = ++lastRevision;
	bound.status = solvers[solverIndex]->Solve(BuildBindContext(bound, mesh), bound.weights);
}

bool SkinningController::Rebuild(BoundMesh& bound, Object3D& mesh, RebuildScope scope)
{
	if (scope == RebuildScope::BindPose)
	{
		// Where the mesh sits inside the rig at the moment of binding: everything
		// the rig does later is measured against this.
		XMStoreFloat4x4(&bound.meshBindToRig,
			mesh.GetWorldMatrix() * XMMatrixInverse(nullptr, rig.GetWorldMatrix()));
	}

	if (scope != RebuildScope::Deformer)
		SolveWeights(bound, mesh);

	if (scope == RebuildScope::Deformer || !bound.deformer)
		bound.deformer = CreateDeformer();

	const bool prepared = !bound.weights.IsEmpty()
		&& bound.deformer->Prepare(bound.restVertices, bound.weights, InfluenceColors(bound));

	mesh.SetDeformedVertices(prepared ? bound.deformer->GetDeformedVertices() : nullptr);
	if (prepared)
		ApplyWeightView(bound, mesh); // the columns, and the focused one's index, may have changed
	else
		RestoreShaders(bound, mesh);
	return prepared;
}

void SkinningController::RebuildAll(RebuildScope scope)
{
	for (BoundMesh& bound : boundMeshes)
	{
		if (const std::shared_ptr<Object3D> mesh = bound.mesh.lock())
			Rebuild(bound, *mesh, scope);
	}
}

// ---------------------------------------------------------------- lifecycle

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

	if (!Rebuild(bound, *mesh, RebuildScope::BindPose))
	{
		return false;
	}

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
	RebuildAll(RebuildScope::BindPose);
}

// ---------------------------------------------------------------- per frame

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
		it->deformer->Deform(palette);
		++it;
	}
}
