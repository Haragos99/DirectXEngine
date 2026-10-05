#pragma once
#include "object3d.h"

#include "bone.h"
#include "joint.h"
#include "jointhandle.h"
#include "skeletongeometry.h"
#include "skeletonpose.h"
#include "skinningcontroller.h"

#include <memory>
#include <vector>

// Scene object that owns a joint hierarchy and the bones between the joints.
// A new skeleton is nothing but a root joint: children are added one at a time
// from the UI, so a rig is built in the editor instead of being hard coded.
//
// Every joint is mirrored by a JointHandle parented into the scene hierarchy,
// which is what makes joints show up and behave like any other scene object.
class Skeleton : public Object3D
{
public:
	Skeleton(Microsoft::WRL::ComPtr<ID3D11Device> _device,
	         Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	void Update(float time) override;

	// Adds a child under `parentJointId` and returns its handle.
	std::shared_ptr<JointHandle> AddJoint(int parentJointId);
	void ResetPose();

	// Joint access used by the handles.
	bool IsValidJoint(int jointId) const;
	const Joint& GetJoint(int jointId) const;
	size_t GetJointCount() const { return joints.size(); }
	const std::vector<Bone>& GetBones() const { return bones; }
	const DirectX::XMFLOAT4X4& GetJointGlobalMatrix(int jointId) const;
	DirectX::XMFLOAT3 GetJointWorldPosition(int jointId) const;
	void SetJointRotation(int jointId, const DirectX::XMFLOAT3& eulerRadians);
	void SetJointOffset(int jointId, const DirectX::XMFLOAT3& parentSpaceOffset);
	// Places a joint inside its parent. While the rig is still unbound this
	// reshapes the rest pose; once a mesh is bound it poses the joint instead,
	// which is what makes the mesh deform.
	void SetJointTranslation(int jointId, const DirectX::XMFLOAT3& parentSpaceTranslation);
	// Moves a joint by a world space delta, as dragged on the gizmo.
	void MoveJoint(int jointId, const DirectX::XMFLOAT3& worldDelta);
	// Joint drawn in the selection colour. -1 clears the highlight.
	void SetSelectedJoint(int jointId);
	// Selecting the rig is selecting its root joint.
	void OnSelected(bool selected) override;

	// Meshes bound to this rig are deformed through here.
	SkinningController& GetSkinning() { return skinning; }
	int GetSkeletonNumber() const { return skeletonNumber; }
	// Joint placement in the rest pose, which is what a mesh is bound against.
	const std::vector<DirectX::XMFLOAT4X4>& GetBindGlobals() const { return pose.GetBindGlobals(); }
	// One matrix per joint (inverse bind * global): the palette a skinning
	// technique multiplies the bound vertices with.
	const std::vector<DirectX::XMFLOAT4X4>& GetSkinningMatrices() const;

protected:
	void createTexturedVertex() override;
	void createIndeces() override;

private:
	void CreateRenderResources();
	// Mirrors every joint with a handle parented into the scene hierarchy.
	void EnsureJointHandles();
	void SyncJointHandles();
	// Re-evaluates the pose and re-uploads the mesh after a change.
	void RefreshIfDirty();
	// The rest pose is what the joint offsets describe with no rotation, so it
	// has to be recaptured whenever the hierarchy or an offset changes.
	void RecaptureBindPose();
	bool HasBoundMesh() const;

	std::vector<Joint> joints;
	std::vector<Bone> bones;
	std::vector<std::shared_ptr<JointHandle>> jointHandles;
	SkeletonPose pose;
	SkeletonGeometry geometry;
	SkinningController skinning;
	int skeletonNumber = 0;
	int selectedJoint = -1;
	bool poseDirty = false;
};
