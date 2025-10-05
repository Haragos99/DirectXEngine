#include "texture.h"
#include "../External/WICTextureLoader.h"
#include <stdexcept>


Texture::Texture(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex) : device(_device), contex(_contex)
{
	CreateSamplerState();
}

void Texture::Use()
{
	
	contex->PSSetSamplers(0, 1, m_SamplerState.GetAddressOf());
	contex->PSSetShaderResources(0, 1, m_DiffuseTexture.GetAddressOf());
}


void Texture::LoadTextureFromFile(std::wstring filename)
{
	ComPtr<ID3D11Resource> resource = nullptr;
	DirectX::CreateWICTextureFromFile(device.Get(), contex.Get(), filename.c_str(), resource.ReleaseAndGetAddressOf(), m_DiffuseTexture.ReleaseAndGetAddressOf());
}



void Texture::CreateSamplerState()
{
	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER_ANISOTROPIC;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MipLODBias = 0;
	desc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
	desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	desc.MinLOD = 0;
	desc.MaxLOD = 1000.0f;

	device->CreateSamplerState(&desc, m_SamplerState.GetAddressOf());
}


void Texture::CreateSolidColorTexture(const DirectX::XMFLOAT4& color)
{
    // Create a 1x1 texture with a single color
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Convert color from float4 [0..1] → byte RGBA [0..255]
    UINT rgba =
        (UINT(color.w * 255.0f) << 24) |
        (UINT(color.z * 255.0f) << 16) |
        (UINT(color.y * 255.0f) << 8) |
        (UINT(color.x * 255.0f));

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &rgba;
    initData.SysMemPitch = sizeof(UINT);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, tex.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create solid color texture!");

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, m_DiffuseTexture.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create SRV for solid color texture!");
}
