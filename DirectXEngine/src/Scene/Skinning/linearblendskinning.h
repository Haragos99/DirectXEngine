#pragma once
#include "iskinningtechnique.h"

#include <wrl/client.h>

class LinearBlendSkinning : public ISkinningTechnique
{
public:
	LinearBlendSkinning(Microsoft::WRL::ComPtr<ID3D11Device> _device,
	                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	const char* GetName() const override { return "Linear Blend"; }

	bool Prepare(const std::vector<VertexData>& restVertices,
	             const SkinWeightTable& weights) override;
	void Deform(const std::vector<DirectX::XMFLOAT4X4>& palette) override;
	void SetWeightDebug(bool enabled) override;
	ID3D11Buffer* GetDeformedVertices() const override { return deformedVertices.Get(); }

private:
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

	UINT vertexCount = 0;
	UINT jointCount = 0;
	bool weightDebug = false;
};
