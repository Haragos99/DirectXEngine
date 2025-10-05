#pragma once
#include <d3d11.h>
#include <wrl\client.h>
#include <string>
#include <DirectXMath.h>
using Microsoft::WRL::ComPtr;
class Texture
{


public:
	Texture(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex);
	virtual ~Texture() = default;
	void LoadTextureFromFile(std::wstring filename);
	void CreateSolidColorTexture(const DirectX::XMFLOAT4& color);
	// Bind texture sampler to pipeline
	void Use();

private:
	ComPtr<ID3D11SamplerState> m_SamplerState;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> contex;
	ComPtr<ID3D11ShaderResourceView> m_DiffuseTexture;
	void CreateSamplerState();
};