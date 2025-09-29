#include "envcube.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include "../External/DDSTextureLoader.h"
EnvCube::EnvCube(Microsoft::WRL::ComPtr<ID3D11Device> gfx, Microsoft::WRL::ComPtr<ID3D11DeviceContext> fx) 
{

    std::wstring name =  L"..\\Resources\\cloudy_skybox.dds";

    // Initialize cube vertex & index buffers same as Object3D (reuse your cube data)
    HRESULT hr = DirectX::CreateDDSTextureFromFile(
        gfx.Get(), fx.Get(),
        name.c_str(),
        nullptr,
        &cubeMapSRV
    );


    if (FAILED(hr))
    {

        throw std::runtime_error("CUBE compilation failed");
    }

    std::wstring shaderPathV = L"shaders\\EnvVertexShader.hlsl";
    std::wstring shaderPathP = L"shaders\\EnvPixelShader.hlsl";

    // Load environment shaders
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob;
    D3DCompileFromFile(shaderPathV.c_str(), nullptr, nullptr, "VSMain", "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsBlob, nullptr);
    D3DCompileFromFile(shaderPathP.c_str(), nullptr, nullptr, "PSMain", "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, nullptr);



    // Define a simple triangle
    Vertex vertices[] =
    {
        // 8 corners of a cube
       { {-1.0f, 1.0f, -1.0f} },
        { {1.0f, 1.0f, -1.0f}},
        { {1.0f, 1.0f, 1.0f} },
        { {-1.0f, 1.0f, 1.0f} },
        { {-1.0f, -1.0f, -1.0f} },
        { {1.0f, -1.0f, -1.0f}},
        { {1.0f, -1.0f, 1.0f} },
        { {-1.0f, -1.0f, 1.0f} },
    };


    // 12 triangles  36 indices
    UINT indices[] =
    {
    3,1,0,
    2,1,3,

    0,5,4,
    1,5,0,

    3,4,7,
    0,4,3,

    1,6,5,
    2,6,1,

    2,7,6,
    3,7,2,

    6,4,5,
    7,4,6,
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
     hr = gfx->CreateBuffer(&bd, &initData, &vertexBuffer);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create vertex buffer");

 


    if (FAILED(hr))
        throw std::runtime_error("Failed to create sampler state");


    // Index Buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices;


    gfx->CreateBuffer(&ibd, &iinitData, &indexBuffer);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create index buffer");





    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT number_elements = ARRAYSIZE(layout);
    gfx->CreateInputLayout(layout, number_elements, vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), &inputLayout);



    gfx->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    gfx->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);


    // Create world constant buffer
    D3D11_BUFFER_DESC bdr = {};
    bdr.Usage = D3D11_USAGE_DEFAULT;
    bdr.ByteWidth = sizeof(ModelViewProjectionBuffer);
    bdr.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    gfx->CreateBuffer(&bdr, nullptr, matrixBuffer.ReleaseAndGetAddressOf());


    D3D11_RASTERIZER_DESC rasterizerState = {};
    rasterizerState.CullMode = D3D11_CULL_BACK;
    rasterizerState.FillMode = D3D11_FILL_SOLID;
    rasterizerState.DepthClipEnable = true;
    rasterizerState.FrontCounterClockwise = true;


    gfx->CreateRasterizerState(&rasterizerState, &raster);



    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
    depth_stencil_desc.DepthEnable = true;
    depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    
    gfx->CreateDepthStencilState(&depth_stencil_desc, &m_DepthStencilState);


    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;   // trilinear filtering
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = gfx->CreateSamplerState(&sampDesc, &samplerState);

}
void EnvCube::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> gfx, Camera camera)
{
    auto postion = camera.getPos();
    DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(postion.x, postion.y, postion.z);
    DirectX::XMMATRIX projection = camera.GetProjectionMatrix();
    DirectX::XMMATRIX view = camera.GetViewMatrix();
    matrix *= world;
    matrix *= view;
    matrix *= projection;

    const int constant_buffer_slot = 0;
    gfx->VSSetConstantBuffers(constant_buffer_slot, 1, matrixBuffer.GetAddressOf());
    gfx->RSSetState(raster.Get());

    // Set depth stencil
    gfx->OMSetDepthStencilState(m_DepthStencilState.Get(), 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    gfx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    gfx->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    gfx->IASetInputLayout(inputLayout.Get());
    gfx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);



    ModelViewProjectionBuffer buffer = {};

    buffer.modelViewProjection = DirectX::XMMatrixTranspose(matrix);

    gfx->UpdateSubresource(matrixBuffer.Get(), 0, nullptr, &buffer, 0, 0);


   

    gfx->VSSetShader(vertexShader.Get(), nullptr, 0);
    gfx->PSSetShader(pixelShader.Get(), nullptr, 0);
    // Bind cube map and sampler
    gfx->PSSetShaderResources(0, 1, cubeMapSRV.GetAddressOf());
    gfx->DrawIndexed(36, 0, 0);
}


