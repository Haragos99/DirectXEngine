#include "object3d.h"
#include <stdexcept>
#include <d3dcompiler.h>
using namespace DirectX;

Object3D::Object3D(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contexx) : world(DirectX::XMMatrixIdentity()), device(_device), context(_contexx)
{
	shader = std::make_unique<Shader>(device, context);
	texture = std::make_unique<Texture>(device, context);
	name = "Unnamed Object";
}

void Object3D::SetPosition(float x, float y, float z)
{
	transform.translation = XMFLOAT3(x, y, z);
	rebuildWorld();
}

void Object3D::SetRotation(float pitch, float yaw, float roll)
{
	transform.rotation = XMFLOAT3(pitch, yaw, roll);
	rebuildWorld();
}

void Object3D::SetScale(float sx, float sy, float sz)
{
	transform.scale = XMFLOAT3(sx, sy, sz);
	rebuildWorld();
}

void Object3D::SetTransform(const Transform& newTransform)
{
	transform = newTransform;
	rebuildWorld();
}

void Object3D::AdjustPosition(float dx, float dy, float dz)
{
	transform.translation.x += dx;
	transform.translation.y += dy;
	transform.translation.z += dz;
	rebuildWorld();
}

void Object3D::Rotate(float pitch, float yaw, float roll)
{
	transform.rotation.x += pitch;
	transform.rotation.y += yaw;
	transform.rotation.z += roll;
	rebuildWorld();
}

void Object3D::Scale(float sx, float sy, float sz)
{
	transform.scale.x *= sx;
	transform.scale.y *= sy;
	transform.scale.z *= sz;
	rebuildWorld();
}

DirectX::XMFLOAT3 Object3D::GetPosition() const
{
	return transform.translation;
}

DirectX::XMFLOAT3 Object3D::GetRotation() const
{
	return transform.rotation;
}

DirectX::XMFLOAT3 Object3D::GetScale() const
{
	return transform.scale;
}

void Object3D::rebuildWorld()
{
	world = transform.ToMatrix();
	if (const std::shared_ptr<Object3D> owner = parent.lock())
		world = world * owner->world;

	updateWorldBoundingBox();

	for (const std::shared_ptr<Object3D>& child : children)
		child->rebuildWorld();
}

void Object3D::setWorld(DirectX::FXMMATRIX newWorld)
{
	world = newWorld;
	updateWorldBoundingBox();

	for (const std::shared_ptr<Object3D>& child : children)
		child->rebuildWorld();
}

void Object3D::AttachChild(std::shared_ptr<Object3D> child)
{
	if (!child || child.get() == this || IsDescendantOf(child.get()))
		return;

	child->parent = weak_from_this();
	children.push_back(child);
	child->rebuildWorld();
}

std::shared_ptr<Object3D> Object3D::DetachChild(const Object3D* child)
{
	for (auto it = children.begin(); it != children.end(); ++it)
	{
		if (it->get() != child)
			continue;

		std::shared_ptr<Object3D> detached = *it;
		children.erase(it);
		detached->parent.reset();
		detached->rebuildWorld();
		return detached;
	}
	return nullptr;
}

bool Object3D::IsDescendantOf(const Object3D* candidate) const
{
	for (std::shared_ptr<Object3D> owner = parent.lock(); owner; owner = owner->parent.lock())
	{
		if (owner.get() == candidate)
			return true;
	}
	return false;
}

void Object3D::UpdateHierarchy(float time)
{
	Update(time);
	for (const std::shared_ptr<Object3D>& child : children)
		child->UpdateHierarchy(time);
}

void Object3D::DrawHierarchy(Camera camera, RenderMode mode)
{
	Draw(camera, mode);
	for (const std::shared_ptr<Object3D>& child : children)
		child->DrawHierarchy(camera, mode);
}


void Object3D::createWorldBoundingBox()
{
	if (vertices.empty())
	{
		return;
	}
	// Local-space axis-aligned bounds built once from the mesh vertices.
	BoundingBox aabb;
	BoundingBox::CreateFromPoints(aabb, vertices.size(), &vertices[0].position, sizeof(VertexData));
	// Keep an oriented box in object space so rotation/scale is respected when transformed.
	BoundingOrientedBox::CreateFromBoundingBox(localBox, aabb);
	updateWorldBoundingBox();
}

void Object3D::updateWorldBoundingBox()
{
	localBox.Transform(worldBox, world);
}

bool Object3D::Intersect(const Ray& ray, float& outDistance)
{
	// Keep the world bounding box in sync with the current world matrix before testing.
	updateWorldBoundingBox();

	const XMVECTOR origin = XMLoadFloat3(&ray.origin);
	const XMVECTOR direction = XMLoadFloat3(&ray.direction);

	float dist = 0.0f;
	if (worldBox.Intersects(origin, direction, dist))
	{
		outDistance = dist;
		return true;
	}
	return false;
}


void Object3D::wireframeOverlay()
{
	context->RSSetState(shader->GetwireframeRS().Get());
	context->PSSetShader(shader->GetBlackPixelShader().Get(), nullptr, 0);
	context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void Object3D::solidOverlay(Camera camera)
{
	DirectX::XMMATRIX projection = camera.GetProjectionMatrix();
	DirectX::XMMATRIX view = camera.GetViewMatrix();
	texture->Use();
	MatrixBuffer mb;
	mb.world = DirectX::XMMatrixTranspose(world);
	mb.view = DirectX::XMMatrixTranspose(view);
	mb.projection = DirectX::XMMatrixTranspose(projection);
	LightBuffer lightData;
	lightData.lightDirection = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f); // example
	lightData.padding = 0.0f;
	lightData.lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // white light
	shader->renderDraw(mb, lightData);

}

void Object3D::Draw(Camera camera, RenderMode mode)
{
	switch (mode)
	{
	case RenderMode::Solid:
		solidOverlay(camera); // sets VB/IB/layout/VS/PS/solid RS/constants
		context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
		break;

	case RenderMode::WireframeOnly:
		solidOverlay(camera); // prepare full pipeline first
		wireframeOverlay();   // then override RS + PS for wireframe
		break;

	case RenderMode::SolidWireframe:
		solidOverlay(camera);
		context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0); // solid pass first
		wireframeOverlay(); // wire pass second
		break;

	default:
		throw std::exception("Invalid Render mode");
	}
}


