#include "skeleton.h"
#include "linearblendskinning.h"

using namespace DirectX;

namespace
{
	// Where a freshly added joint appears relative to its parent.
	const XMFLOAT3 kNewJointOffset(0.0f, 0.4f, 0.0f);
}

Skeleton::Skeleton(Microsoft::WRL::ComPtr<ID3D11Device> _device,
                   Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
	: Object3D(_device, _context)
{
	joints.emplace_back("Root", 0, Joint::kNoParent, XMFLOAT3(0.0f, 0.0f, 0.0f));
	skinningTechnique = std::make_shared<LinearBlendSkinning>();

	RecaptureBindPose();
	CreateRenderResources();
	name = "Skeleton";
}

void Skeleton::CreateRenderResources()
{
	createTexturedVertex();
	createIndeces();

	// The skeleton shaders carry their colour per vertex, but the shared render
	// path still binds a texture, so give it a neutral one.
	texture->CreateSolidColorTexture(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	shader->createConstantBuffer();
	shader->creaetLightBuffer();
	shader->createRasterize();
	shader->LoadShaders(L"shaders\\SkeletonVertexShader.hlsl",
	                    L"shaders\\SkeletonPixelShader.hlsl");

	createWorldBoundingBox();
	wireframeEnabled = false;
	poseDirty = false;
}

void Skeleton::createTexturedVertex()
{
	geometry.Build(joints, bones, pose, selectedJoint);
	vertices = geometry.GetVertices();
	indices = geometry.GetIndices();
}

void Skeleton::createIndeces()
{
	// Indices are produced together with the vertices by SkeletonGeometry.
}

void Skeleton::Update(float /*time*/)
{
	EnsureJointHandles();
	RefreshIfDirty();
	SyncJointHandles();
}

// A handle needs the skeleton to be owned by a shared_ptr already, so handles
// are created here rather than in the constructor.
void Skeleton::EnsureJointHandles()
{
	for (size_t id = jointHandles.size(); id < joints.size(); ++id)
	{
		auto handle = std::make_shared<JointHandle>(*this, static_cast<int>(id));
		jointHandles.push_back(handle);

		const int parentId = joints[id].GetParentId();
		if (parentId == Joint::kNoParent)
			AttachChild(handle);
		else
			jointHandles[parentId]->AttachChild(handle);
	}
}

void Skeleton::SyncJointHandles()
{
	for (const std::shared_ptr<JointHandle>& handle : jointHandles)
		handle->SyncToJoint(world);
}

void Skeleton::RefreshIfDirty()
{
	if (!poseDirty)
		return;

	pose.Evaluate(joints);
	createTexturedVertex();
	shader->createVertexBuffer(vertices);
	shader->createInexxBuffer(indices);
	createWorldBoundingBox();
	poseDirty = false;
}

void Skeleton::RecaptureBindPose()
{
	pose.CaptureBindPose(joints);
	pose.Evaluate(joints);
	poseDirty = true;
}

bool Skeleton::IsValidJoint(int jointId) const
{
	return jointId >= 0 && jointId < static_cast<int>(joints.size());
}

const Joint& Skeleton::GetJoint(int jointId) const
{
	return joints[static_cast<size_t>(jointId)];
}

const XMFLOAT4X4& Skeleton::GetJointGlobalMatrix(int jointId) const
{
	return pose.GetGlobalMatrix(jointId);
}

std::shared_ptr<JointHandle> Skeleton::AddJoint(int parentJointId)
{
	if (!IsValidJoint(parentJointId))
		return nullptr;

	EnsureJointHandles(); // the parent handle has to exist before we hang off it

	const int jointId = static_cast<int>(joints.size());
	joints.emplace_back("Joint " + std::to_string(jointId), jointId, parentJointId, kNewJointOffset);
	joints[parentJointId].AddChild(jointId);

	const int boneId = static_cast<int>(bones.size());
	bones.emplace_back(boneId, parentJointId, jointId, BoneColors::For(boneId));

	RecaptureBindPose();
	EnsureJointHandles();
	return jointHandles[static_cast<size_t>(jointId)];
}

void Skeleton::SetJointRotation(int jointId, const XMFLOAT3& eulerRadians)
{
	if (!IsValidJoint(jointId))
		return;

	joints[jointId].SetRotation(eulerRadians);
	poseDirty = true;
}

void Skeleton::SetJointOffset(int jointId, const XMFLOAT3& parentSpaceOffset)
{
	if (!IsValidJoint(jointId))
		return;

	joints[jointId].SetBindOffset(parentSpaceOffset);
	RecaptureBindPose();
}

void Skeleton::MoveJoint(int jointId, const XMFLOAT3& worldDelta)
{
	if (!IsValidJoint(jointId))
		return;

	const Joint& joint = joints[jointId];

	// A joint stores its offset in its parent's space, so the world space drag
	// has to be pulled back through the parent chain and the object transform.
	XMMATRIX parentToWorld = world;
	if (!joint.IsRoot())
		parentToWorld = XMLoadFloat4x4(&pose.GetGlobalMatrix(joint.GetParentId())) * world;

	const XMVECTOR localDelta = XMVector3TransformNormal(
		XMLoadFloat3(&worldDelta), XMMatrixInverse(nullptr, parentToWorld));

	XMFLOAT3 offset;
	XMStoreFloat3(&offset, XMVectorAdd(XMLoadFloat3(&joint.GetBindOffset()), localDelta));
	SetJointOffset(jointId, offset);
}

void Skeleton::ResetPose()
{
	for (Joint& joint : joints)
		joint.ResetPose();

	poseDirty = true;
}

void Skeleton::SetSelectedJoint(int jointId)
{
	const int clamped = IsValidJoint(jointId) ? jointId : -1;
	if (clamped == selectedJoint)
		return;

	selectedJoint = clamped;
	poseDirty = true; // the highlight colour is baked into the mesh
}

XMFLOAT3 Skeleton::GetJointWorldPosition(int jointId) const
{
	const XMFLOAT3 local = pose.GetJointPosition(jointId);

	XMFLOAT3 result;
	XMStoreFloat3(&result, XMVector3Transform(XMLoadFloat3(&local), world));
	return result;
}

void Skeleton::SetSkinningTechnique(std::shared_ptr<ISkinningTechnique> technique)
{
	if (technique)
		skinningTechnique = std::move(technique);
}

const std::vector<XMFLOAT4X4>& Skeleton::GetSkinningMatrices() const
{
	return pose.GetSkinningMatrices();
}
