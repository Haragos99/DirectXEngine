#include "raytraceshader.h"
#include <stdexcept>
#include <d3dcompiler.h>

RaytraceShader::RaytraceShader(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext>  _context) : Shader(_device, _context)
{

}


void RaytraceShader::LoadShaders(const std::wstring& vertexShaderPath, const std::wstring& pixelShaderPath)
{
	// Load Shaders
	loadPixelShader(pixelShaderPath);
	loadVertexShader(vertexShaderPath);
	//loadwireframePixelShader();
}


void RaytraceShader::createVertexRTBuffer(std::vector<VertexRT> vertices)
{
    // VB
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(VertexRT) * vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = { vertices.data() };
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &vertexBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Vertex Buffer creation failed");
	}
}

void RaytraceShader::loadVertexShader(const std::wstring& vertexShaderPath)
{
    // Compile shaders
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, err;

    HRESULT hr = D3DCompileFromFile(vertexShaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &err);

    if (FAILED(hr))
    {
        throw std::runtime_error("Vertex Shader compilation failed");
    }

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
   
    // Layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
}



void RaytraceShader::creatQuadricsBuffer()
{
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
}


void RaytraceShader::createCameraBuffer()
{
    // Camera CB
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(CameraCB);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    device->CreateBuffer(&cbd, nullptr, &cameraBuffer);
}

void RaytraceShader::renderRaytraceDraw(CameraCB& cb, SphereBuffer& sb)
{
    UINT stride = sizeof(VertexRT), offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(inputLayout.Get());

    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    context->UpdateSubresource(cameraBuffer.Get(), 0, nullptr, &cb, 0, 0);
    context->PSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());

    context->UpdateSubresource(sphereCBuffer.Get(), 0, nullptr, &sb, 0, 0);
    context->PSSetConstantBuffers(1, 1, sphereCBuffer.GetAddressOf());

}