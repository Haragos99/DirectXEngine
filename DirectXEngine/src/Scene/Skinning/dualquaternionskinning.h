#pragma once
#include "iskinningtechnique.h"

#include <wrl/client.h>

// GPU dual quaternion skinning. Blends rigid transforms rather than matrices,
// so a twisted joint keeps its volume where linear blending pinches it.
//
// A dual quaternion carries no scale, so any scale in the palette is dropped
// when the matrices are converted.
class DualQuaternionSkinning : public ISkinningTechnique
{
public:
	DualQuaternionSkinning(Microsoft::WRL::ComPtr<ID3D11Device> _device,
	                       Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	const char* GetName() const override { return "Dual Quaternion"; }

	bool Prepare(const std::vector<VertexData>& restVertices,
	             const SkinWeightTable& weights) override;
	void Deform(const std::vector<DirectX::XMFLOAT4X4>& palette) override;
	void SetWeightDebug(bool enabled) override;
	ID3D11Buffer* GetDeformedVertices() const override { return deformedVertices.Get(); }

private:
	// Mirrors DualQuat in DualQuaternionSkinningCS.hlsl.
	struct DualQuaternion
	{
		DirectX::XMFLOAT4 real;
		DirectX::XMFLOAT4 dual;
	};

	bool LoadShader();
	void UpdateConstants();
	// Read-only structured input the dispatch samples per vertex.
	bool CreateInput(const void* data, UINT stride, UINT count,
	                 Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer,
	                 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& view) const;
	// The mesh's vertex buffer, also writable as a raw UAV.
	bool CreateOutput(const std::vector<VertexData>& restVertices);

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> computeShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> restBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> restView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> weightBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> weightView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> paletteBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paletteView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer> deformedVertices;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> deformedView;

	// Rebuilt every frame; kept as a member so the upload allocates nothing.
	std::vector<DualQuaternion> dualQuaternions;

	UINT vertexCount = 0;
	UINT jointCount = 0;
	bool weightDebug = false;
};
