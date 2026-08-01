#pragma once
#include <DirectXMath.h>
struct VertexData
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texcoord;

};

struct LightBuffer
{
	DirectX::XMFLOAT3 lightDirection;
	float padding;
	DirectX::XMFLOAT4 lightColor;
};

struct MatrixBuffer
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

struct VertexRT
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 uv;
};

struct CameraCB
{
    DirectX::XMMATRIX invViewProj;
    DirectX::XMFLOAT3 camPos;
    float padding;
};

struct Sphere
{
	DirectX::XMFLOAT3 center; 
	float radius;             
};

struct SphereBuffer
{
	Sphere spheres[16];
	int sphereCount;
	float padding[3];
};