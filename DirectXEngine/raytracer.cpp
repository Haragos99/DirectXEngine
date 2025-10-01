#include "raytracer.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include "../External/DDSTextureLoader.h"

using namespace DirectX;

Raytracer::Raytracer(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context) : device(_device), context(_context)
{
    shaderRay = std::make_unique<RaytraceShader>(device, context);
    // Fullscreen quad
    std::vector<VertexRT> vertices 
    {
        { {-1, -1, 0}, {0, 1} },
        { {-1,  1, 0}, {0, 0} },
        { { 1,  1, 0}, {1, 0} },
        { { 1, -1, 0}, {1, 1} },
    };
    createIndeces();
	shaderRay->LoadShaders(L"shaders\\RaytracerVS.hlsl", L"shaders\\RaytracerPS.hlsl");
    shaderRay->createVertexRTBuffer(vertices);
    shaderRay->createInexxBuffer(indices);
	shaderRay->createCameraBuffer();
    shaderRay->creatQuadricsBuffer();
}


void Raytracer::createIndeces()
{
    indices = {
        0,1,2, 0,2,3
	};
}
void Raytracer::Update(float time)
{

}


SphereBuffer Raytracer::initElemt()
{
    SphereBuffer sb = {};
    sb.sphereCount = 16;

    // Sphere 1
    sb.spheres[0].center = DirectX::XMFLOAT3(0.0f, 12.0f, 2.0f);
    sb.spheres[0].radius = 1.0f;

    // Sphere 2
    sb.spheres[1].center = DirectX::XMFLOAT3(2.0f, 10.0f, 4.0f);
    sb.spheres[1].radius = 0.5f;

    // Sphere 3
    sb.spheres[2].center = DirectX::XMFLOAT3(-2.0f, -1.0f, 3.0f);
    sb.spheres[2].radius = 0.75f;

    // Sphere 4
    sb.spheres[3].center = DirectX::XMFLOAT3(1.5f, -1.0f, 5.0f);
    sb.spheres[3].radius = 1.25f;

    // Sphere 5
    sb.spheres[4].center = DirectX::XMFLOAT3(0.0f, -2.5f, 6.0f);
    sb.spheres[4].radius = 0.9f;

    // Sphere 6
    sb.spheres[5].center = DirectX::XMFLOAT3(-3.0f, -1.5f, 2.0f);
    sb.spheres[5].radius = 1.1f;

    // Sphere 7
    sb.spheres[6].center = DirectX::XMFLOAT3(3.0f, -1.0f, 7.0f);
    sb.spheres[6].radius = 0.6f;

    // Sphere 8
    sb.spheres[7].center = DirectX::XMFLOAT3(1.0f, -3.0f, 4.0f);
    sb.spheres[7].radius = 1.4f;

    // Sphere 9
    sb.spheres[8].center = DirectX::XMFLOAT3(-1.0f, -2.0f, 5.0f);
    sb.spheres[8].radius = 0.8f;

    // Sphere 10
    sb.spheres[9].center = DirectX::XMFLOAT3(2.5f, -0.5f, 3.5f);
    sb.spheres[9].radius = 1.2f;

    // Sphere 11
    sb.spheres[10].center = DirectX::XMFLOAT3(-2.5f, -2.0f, 6.5f);
    sb.spheres[10].radius = 0.7f;

    // Sphere 12
    sb.spheres[11].center = DirectX::XMFLOAT3(0.5f, -3.0f, 2.5f);
    sb.spheres[11].radius = 1.3f;

    // Sphere 13
    sb.spheres[12].center = DirectX::XMFLOAT3(3.5f, 0.0f, 5.5f);
    sb.spheres[12].radius = 0.95f;

    // Sphere 14
    sb.spheres[13].center = DirectX::XMFLOAT3(-1.5f, -1.5f, 3.0f);
    sb.spheres[13].radius = 1.05f;

    // Sphere 15
    sb.spheres[14].center = DirectX::XMFLOAT3(2.0f, -2.0f, 6.0f);
    sb.spheres[14].radius = 0.85f;

    // Sphere 16
    sb.spheres[15].center = DirectX::XMFLOAT3(0.0f, -4.0f, 8.0f);
    sb.spheres[15].radius = 1.5f;

	return sb;
}


void Raytracer::Draw(Camera camera)
{
	auto view = camera.GetViewMatrix();
	auto proj = camera.GetProjectionMatrix();
	auto camPos = camera.getPos();

    // Compute inverse view-projection
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    CameraCB cb;
    XMStoreFloat3(&cb.camPos, XMLoadFloat3(&camPos));
    cb.invViewProj = XMMatrixTranspose(invViewProj);

	SphereBuffer sb = initElemt();

	shaderRay->renderRaytraceDraw(cb, sb);

    context->DrawIndexed(6, 0, 0);
}
