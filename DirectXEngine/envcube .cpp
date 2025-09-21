#include "envcube.h"
#include <d3dcompiler.h>
EnvCube::EnvCube(Microsoft::WRL::ComPtr<ID3D11Device> gfx,
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMap)
    : cubeMapSRV(cubeMap)
{
    // Initialize cube vertex & index buffers same as Object3D (reuse your cube data)

    // Load environment shaders
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob;
    D3DCompileFromFile(L"EnvVertexShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsBlob, nullptr);
    D3DCompileFromFile(L"EnvPixelShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, nullptr);

    gfx->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, &vertexShader);
    gfx->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
        nullptr, &pixelShader);

    // Input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    gfx->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), &inputLayout);

    // Camera constant buffer
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(DirectX::XMFLOAT3) + sizeof(float); // for alignment
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    gfx->CreateBuffer(&cbd, nullptr, &cameraBuffer);

    // Sampler state
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    gfx->CreateSamplerState(&sampDesc, &samplerState);
}


void EnvCube::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> gfx, Camera camera)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    gfx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    gfx->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    gfx->IASetInputLayout(inputLayout.Get());
    gfx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Update camera buffer
    struct CameraData { DirectX::XMFLOAT3 pos; float padding; };
    CameraData cb;
    cb.pos = camera.getPos();
    gfx->UpdateSubresource(cameraBuffer.Get(), 0, nullptr, &cb, 0, 0);
    gfx->PSSetConstantBuffers(1, 1, cameraBuffer.GetAddressOf());

    // Bind cube map and sampler
    gfx->PSSetShaderResources(0, 1, cubeMapSRV.GetAddressOf());
    gfx->PSSetSamplers(0, 1, samplerState.GetAddressOf());

    gfx->VSSetShader(vertexShader.Get(), nullptr, 0);
    gfx->PSSetShader(pixelShader.Get(), nullptr, 0);

    gfx->DrawIndexed(36, 0, 0);
}


