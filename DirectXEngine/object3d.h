#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "camera.h"
#include <vector>
#include <memory>
#include "texture.h"
#include "vertex.h"
#include "shaders.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

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
	Object3D(Microsoft::WRL::ComPtr<ID3D11Device> _device , Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex);
	void Update(float time);     
	virtual void Draw(Camera camera);
	bool wireframeEnabled;

protected:
	void createTexturedCube();
	void createIndeces();
	std::unique_ptr <Shader> shader;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>  context;
	std::vector<Vertex> vertices;
	std::vector<UINT>   indices;
	std::unique_ptr<Texture> texture;
	DirectX::XMMATRIX world;
};