#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "camera.h"
#include <vector>
#include <memory>
#include "texture.h"
#include "transform.h"
#include "vertex.h"
#include "shaders.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

enum class RenderMode
{
	Solid,
	WireframeOnly,
	SolidWireframe
};


class Object3D : public std::enable_shared_from_this<Object3D>
{
public:
	Object3D() = default;
	Object3D(Microsoft::WRL::ComPtr<ID3D11Device> _device , Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex);
	// Scene objects are owned and destroyed through Object3D pointers.
	virtual ~Object3D() = default;
	virtual void Update(float time) = 0;
	virtual void Draw(Camera camera, RenderMode mode);
	bool wireframeEnabled;

	// Absolute transform edited by the properties panel. Get/SetTransform are
	// virtual so a proxy object can present something else as its transform.
	void SetPosition(float x, float y, float z);
	void SetRotation(float pitch, float yaw, float roll);
	void SetScale(float sx, float sy, float sz);
	virtual void SetTransform(const Transform& newTransform);

	// Relative transform used by the gizmos while dragging. AdjustPosition is
	// virtual so a proxy object can redirect a drag somewhere else.
	virtual void AdjustPosition(float dx, float dy, float dz);
	void Rotate(float pitch, float yaw, float roll);
	void Scale(float sx, float sy, float sz);

	virtual Transform GetTransform() const { return transform; }
	virtual DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMFLOAT3 GetRotation() const;
	DirectX::XMFLOAT3 GetScale() const;
	std::string GetName() const { return name; }

	// Scene hierarchy. A child inherits its parent's world transform and is
	// updated, drawn and destroyed through it.
	void AttachChild(std::shared_ptr<Object3D> child);
	// Unlinks `child` and hands its ownership back to the caller.
	std::shared_ptr<Object3D> DetachChild(const Object3D* child);
	const std::vector<std::shared_ptr<Object3D>>& GetChildren() const { return children; }
	std::shared_ptr<Object3D> GetParent() const { return parent.lock(); }
	bool IsDescendantOf(const Object3D* candidate) const;

	// Updates/draws this object and everything parented to it.
	void UpdateHierarchy(float time);
	void DrawHierarchy(Camera camera, RenderMode mode);

	// Called when this object becomes, or stops being, the editor selection.
	virtual void OnSelected(bool /*selected*/) {}

	// Ray/object intersection test in world space (broad-phase, oriented bounding box).
	// Returns true on a hit and writes the distance from the ray origin to outDistance.
	virtual bool Intersect(const Ray& ray, float& outDistance);
protected:
	DirectX::XMVECTOR position;
	virtual void createTexturedVertex() = 0;
	virtual void createIndeces() = 0;
	void wireframeOverlay();
	void solidOverlay(Camera camera);
	void createWorldBoundingBox();
	// Refresh worldBox from the local bounds using the current world matrix.
	void updateWorldBoundingBox();
	// Recomposes the world matrix after the transform components changed.
	void rebuildWorld();
	// Overrides the composed world matrix, for objects that derive it elsewhere.
	void setWorld(DirectX::FXMMATRIX newWorld);
	Transform transform;
	std::weak_ptr<Object3D> parent;
	std::vector<std::shared_ptr<Object3D>> children;
	std::unique_ptr <Shader> shader;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>  context;
	std::vector<VertexData> vertices;
	std::vector<UINT>   indices;
	std::unique_ptr<Texture> texture;
	DirectX::XMMATRIX world;
	std::string name;
	DirectX::BoundingOrientedBox localBox; // object space, built once from vertices
	DirectX::BoundingOrientedBox worldBox; // localBox transformed by the world matrix
};