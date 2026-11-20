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

// One corner of the screen aligned quad that every Gaussian splat is expanded to.
struct SplatCorner
{
	DirectX::XMFLOAT2 corner;
};

// Per-splat payload of a Gaussian splat, held in a GPU structured buffer.
// Members are padded to 16 byte boundaries so the HLSL struct layout matches.
struct SplatInstance
{
	DirectX::XMFLOAT3 center;
	float centerPadding;
	DirectX::XMFLOAT3 scale;
	float scalePadding;
	DirectX::XMFLOAT4 rotation; // (x, y, z, w) unit quaternion
	DirectX::XMFLOAT4 color;    // rgb + opacity
};

// Per-frame values the splat shader needs to project 3D covariances to screen space.
struct SplatFrameBuffer
{
	DirectX::XMFLOAT2 focal;        // focal length in pixels
	DirectX::XMFLOAT2 viewportSize; // in pixels
	float splatScale;               // global size multiplier
	float alphaCutoff;
	DirectX::XMFLOAT2 padding;
};