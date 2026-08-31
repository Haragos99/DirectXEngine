#pragma once
#include "object3d.h"

#include "bone.h"
#include "iskinningtechnique.h"
#include "joint.h"
#include "jointhandle.h"
#include "skeletongeometry.h"
#include "skeletonpose.h"

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
	const DirectX::XMFLOAT4X4& GetJointGlobalMatrix(int jointId) const;
	DirectX::XMFLOAT3 GetJointWorldPosition(int jointId) const;
	void SetJointRotation(int jointId, const DirectX::XMFLOAT3& eulerRadians);
	void SetJointOffset(int jointId, const DirectX::XMFLOAT3& parentSpaceOffset);
	// Moves a joint by a world space delta, as dragged on the gizmo.
	void MoveJoint(int jointId, const DirectX::XMFLOAT3& worldDelta);
	// Joint drawn in the selection colour. -1 clears the highlight.
	void SetSelectedJoint(int jointId);

	// How the rig will deform a bound mesh once skinning exists.
	void SetSkinningTechnique(std::shared_ptr<ISkinningTechnique> technique);
	const std::shared_ptr<ISkinningTechnique>& GetSkinningTechnique() const { return skinningTechnique; }
	// One matrix per joint (global * inverse bind): the palette a skinning
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

	std::vector<Joint> joints;
	std::vector<Bone> bones;
	std::vector<std::shared_ptr<JointHandle>> jointHandles;
	SkeletonPose pose;
	SkeletonGeometry geometry;
	std::shared_ptr<ISkinningTechnique> skinningTechnique;
	int selectedJoint = -1;
	bool poseDirty = false;
};
