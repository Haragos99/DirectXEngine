#include "object3d.h"
#include <stdexcept>
#include <d3dcompiler.h>
using namespace DirectX;

Object3D::Object3D(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contexx) : world(DirectX::XMMatrixIdentity()), device(_device), context(_contexx)
{
	shader = std::make_unique<Shader>(device, context);
	texture = std::make_unique<Texture>(device, context);
}

void Object3D::SetPosition(float x, float y, float z)
{
	world *= DirectX::XMMatrixTranslation(x, y, z);
}

void Object3D::Rotate(float pitch, float yaw, float roll)
{
	world *= DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
}

void Object3D::Scale(float sx, float sy, float sz)
{
	world *= DirectX::XMMatrixScaling(sx, sy, sz);
}
