#pragma once
#include "object3d.h"

class Skeleton;

// A joint seen as a scene object, so the outliner, the properties panel and the
// gizmo can treat joints like anything else. It carries no geometry: every
// transform query and edit is forwarded to the joint it stands for.
class JointHandle : public Object3D
{
public:
	JointHandle(Skeleton& owner, int joint);

	int GetJointId() const { return jointId; }
	Skeleton& GetSkeleton() const { return skeleton; }
	// Re-derives the world matrix from the joint's place in the posed rig, so
	// objects parented to this handle follow the joint.
	void SyncToJoint(DirectX::FXMMATRIX skeletonWorld);

	// Position in world space; transform is the offset from the parent joint
	// plus the pose rotation.
	DirectX::XMFLOAT3 GetPosition() const override;
	void AdjustPosition(float dx, float dy, float dz) override;
	Transform GetTransform() const override;
	void SetTransform(const Transform& newTransform) override;

	void OnSelected(bool selected) override;
	// A handle is a widget, not geometry, so it is never picked in the viewport.
	bool Intersect(const Ray& /*ray*/, float& /*outDistance*/) override { return false; }

	void Update(float /*time*/) override {}
	void Draw(Camera /*camera*/, RenderMode /*mode*/) override {}

protected:
	void createTexturedVertex() override {}
	void createIndeces() override {}

private:
	Skeleton& skeleton;
	int jointId;
};
