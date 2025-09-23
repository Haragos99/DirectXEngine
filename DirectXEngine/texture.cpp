#include "texture.h"
#include "../External/WICTextureLoader.h"


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