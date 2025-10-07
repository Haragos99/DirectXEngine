#pragma once
#include <string>
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "vertex.h"
#include <vector>
#pragma comment(lib, "d3d11.lib")



class Shader
{
public:
	Shader() = default;
	Shader(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext>  _context);
	~Shader() = default;
	virtual void LoadShaders(const std::wstring& vertexShaderPath, const std::wstring& pixelShaderPath);
	Microsoft::WRL::ComPtr<ID3D11VertexShader> GetVertexShader() const { return vertexShader; }
	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader() const { return pixelShader; }
	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() const { return inputLayout; }
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> GetwireframeRS() const { return wireframeRS; }
	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetBlackPixelShader() { return blackPixelShader; }
	template<typename T>
	void createVertexBuffer(std::vector<T> vertices)
	{
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = (vertices.size() * sizeof(T));
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = vertices.data();
		HRESULT hr = device->CreateBuffer(&bd, &initData, &vertexBuffer);
		if (FAILED(hr))
		{
			//throw std::runtime_error("Failed to create vertex buffer");
		}
	}
	void createInexxBuffer(std::vector<UINT> indices);
	void createRasterize();
	void createConstantBuffer();
	void creaetLightBuffer();
	void renderDraw(MatrixBuffer& mb, LightBuffer& lightData);

protected:
	virtual void loadVertexShader(const std::wstring& vertexShaderPath);
	void loadPixelShader(const std::wstring& pixelShaderPath);
	void loadwireframePixelShader();
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>  context;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> lightBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> solidRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> wireframeRS;
private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> blackPixelShader;
};
