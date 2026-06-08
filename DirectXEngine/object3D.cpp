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


void Object3D::wireframeOverlay()
{

	context->RSSetState(shader->GetwireframeRS().Get());
	context->PSSetShader(shader->GetBlackPixelShader().Get(), nullptr, 0);
	context->DrawIndexed(indices.size(), 0, 0);
	
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
	if(mode == RenderMode::Solid || mode == RenderMode::SolidWireframe)
	{
		solidOverlay(camera);
	}
	if(mode == RenderMode::WireframeOnly || mode == RenderMode::SolidWireframe)
	{
		wireframeOverlay();
	}
	context->DrawIndexed(indices.size(), 0, 0);
}


