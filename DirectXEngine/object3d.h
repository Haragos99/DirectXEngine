#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "camera.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

struct Vertex 
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

struct MatrixBuffer
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};


enum class RenderMode
{
	Solid,
	WireframeOnly,
	SolidWireframe
};



class Object3D
{
public:
	Object3D() = default;
	Object3D(Microsoft::WRL::ComPtr<ID3D11Device> gfx);
	void Update(float time);     // animate rotation
	virtual void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> gfx, Camera camera);
	bool wireframeEnabled;

protected:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> wireframeRS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> blackPixelShader;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> solidRS;
	DirectX::XMMATRIX world;
};