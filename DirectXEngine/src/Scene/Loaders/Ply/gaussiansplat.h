#pragma once
#include <DirectXMath.h>
#include <vector>

// One anisotropic 3D Gaussian ("splat") as stored in a 3DGS .ply file.
// Values are already decoded into renderer friendly units: the scale is a
// world space standard deviation per local axis and the colour is linear RGB
// with the opacity in alpha.
struct GaussianSplat
{
	DirectX::XMFLOAT3 center{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale{ 0.01f, 0.01f, 0.01f };
	DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f }; // (x, y, z, w) unit quaternion
	DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };    // rgb from SH band 0, a = opacity
};

// The full decoded content of a splat file.
struct GaussianSplatCloud
{
	std::vector<GaussianSplat> splats;
	DirectX::XMFLOAT3 boundsMin{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 boundsMax{ 0.0f, 0.0f, 0.0f };

	bool Empty() const { return splats.empty(); }
	DirectX::XMFLOAT3 Center() const;
	// Largest bounding box side, used to normalise wildly scaled captures.
	float Extent() const;
	void ComputeBounds();
};
