#include "raytracer.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include "../External/DDSTextureLoader.h"

using namespace DirectX;

Raytracer::Raytracer(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context) : device(_device), context(_context)
{
    // Fullscreen quad
    VertexRT vertices[] =
    {
        { {-1, -1, 0}, {0, 1} },
        { {-1,  1, 0}, {0, 0} },
        { { 1,  1, 0}, {1, 0} },
        { { 1, -1, 0}, {1, 1} },
    };

    UINT indices[] = { 0,1,2, 0,2,3 };

    // VB
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = { vertices };
    device->CreateBuffer(&vbd, &vinit, &vertexBuffer);

    // IB
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = { indices };
    device->CreateBuffer(&ibd, &iinit, &indexBuffer);

    // Camera CB
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(CameraCB);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    device->CreateBuffer(&cbd, nullptr, &cameraBuffer);



    D3D11_BUFFER_DESC sbd = {};
    sbd.Usage = D3D11_USAGE_DEFAULT; // use DEFAULT if calling UpdateSubresource
    sbd.ByteWidth = (UINT)((sizeof(SphereBuffer) + 15) & ~15); // round up to multiple of 16
    sbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    sbd.CPUAccessFlags = 0;
    HRESULT hr = device->CreateBuffer(&sbd, nullptr, &sphereCBuffer);

    if (FAILED(hr))
    {
        throw std::runtime_error("compilation failed");
    }




    // Compile shaders
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, err;

    hr = D3DCompileFromFile(L"shaders\\RaytracerVS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &err);

    D3DCompileFromFile(L"shaders\\RaytracerPS.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &err);
    if (FAILED(hr))
    {
        throw std::runtime_error("Vertex Shader compilation failed");
    }

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

    // Layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
}

void Raytracer::Draw(Camera camera)
{
	auto view = camera.GetViewMatrix();
	auto proj = camera.GetProjectionMatrix();
	auto camPos = camera.getPos();
    UINT stride = sizeof(VertexRT), offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(inputLayout.Get());


    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    // Compute inverse view-projection
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    CameraCB cb;
    XMStoreFloat3(&cb.camPos, XMLoadFloat3(&camPos));
    cb.invViewProj = XMMatrixTranspose(invViewProj);


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



    context->UpdateSubresource(cameraBuffer.Get(), 0, nullptr, &cb, 0, 0);
    context->PSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());

    context->UpdateSubresource(sphereCBuffer.Get(), 0, nullptr, &sb, 0, 0);
    context->PSSetConstantBuffers(1, 1, sphereCBuffer.GetAddressOf());

    context->DrawIndexed(6, 0, 0);
}
