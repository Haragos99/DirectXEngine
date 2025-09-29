#include "raytracer.h"
#include <d3dcompiler.h>
#include <stdexcept>

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

    // Compile shaders
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, err;

    HRESULT hr = D3DCompileFromFile(L"shaders\\RaytracerVS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &err);

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

void Raytracer::Draw(Camera camera, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMapSRV, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState)
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

    context->UpdateSubresource(cameraBuffer.Get(), 0, nullptr, &cb, 0, 0);
    context->PSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());

    context->PSSetShaderResources(0, 1, &cubeMapSRV);
    context->PSSetSamplers(0, 1, &samplerState);


    context->DrawIndexed(6, 0, 0);
}
