#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "camera.h"
#include <vector>
#include <memory>
#include "texture.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

struct Vertex 
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texcoord; 

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
struct LightBuffer
{
	DirectX::XMFLOAT3 lightDirection;
	float padding;
	DirectX::XMFLOAT4 lightColor;
};


class Object3D
{
public:
	Object3D() = default;
	Object3D(Microsoft::WRL::ComPtr<ID3D11Device> _device , Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex);
	void Update(float time);     
	virtual void Draw(Camera camera);
	bool wireframeEnabled;

protected:

	void createVertexBuffer();
	void createInexxBuffer();
	void createRasterize();
	void createConstantBuffer();
	void creaetLightBuffer();
	void createTexturedCube();
	void loadShaders();
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>  context;
	std::vector<Vertex> vertices;
	std::vector<UINT>   indices;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> lightBuffer;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> wireframeRS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> blackPixelShader;
	//TODO: create shader class
	std::unique_ptr<Texture> texture;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> solidRS;
	DirectX::XMMATRIX world;
};