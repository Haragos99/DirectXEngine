#pragma once
#include "iskindeformer.h"

#include <string>
#include <wrl/client.h>

// Everything a compute shader skinning technique shares: the rest vertices,
// weights and influence colours as structured inputs, the deformed vertex
// buffer as a raw UAV, the SkinConstants block of SkinningCommon.hlsli, and the
// dispatch. A technique only names its shader and encodes the palette.
class ComputeSkinDeformer : public ISkinDeformer
{
public:
	bool Prepare(const std::vector<VertexData>& restVertices,
	             const SkinWeightTable& weights,
	             const std::vector<DirectX::XMFLOAT3>& influenceColors) final;
	void Deform(const std::vector<DirectX::XMFLOAT4X4>& palette) final;
	void SetWeightView(SkinWeightView view, int selectedInfluence) final;
	ID3D11Buffer* GetDeformedVertices() const final { return deformedVertices.Get(); }

protected:
	// `paletteStride` is the size of one encoded palette entry, as the shader's
	// gPalette (t2) declares it.
	ComputeSkinDeformer(Microsoft::WRL::ComPtr<ID3D11Device> _device,
	                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context,
	                    std::wstring _shaderPath,
	                    UINT _paletteStride);

	// Called once Prepare has the influence count, before the first Deform.
	virtual void OnPrepared(UINT /*influenceCount*/) {}
	// Turns one matrix per influence into InfluenceCount() entries of
	// `paletteStride` bytes each, ready to upload.
	virtual const void* EncodePalette(const std::vector<DirectX::XMFLOAT4X4>& palette) = 0;

	UINT InfluenceCount() const { return influenceCount; }

private:
	bool LoadShader();
	void UpdateConstants();
	void Release();
	// Read-only structured input the dispatch samples per vertex.
	bool CreateInput(const void* data, UINT stride, UINT count,
	                 Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer,
	                 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& view) const;
	// The mesh's vertex buffer, also writable as a raw UAV.
	bool CreateOutput(const std::vector<VertexData>& restVertices);

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	std::wstring shaderPath;
	UINT paletteStride;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> computeShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> restBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> restView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> weightBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> weightView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> paletteBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paletteView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> colorBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> colorView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer> deformedVertices;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> deformedView;

	UINT vertexCount = 0;
	UINT influenceCount = 0;
	SkinWeightView weightViewMode = SkinWeightView::Off;
	int selectedInfluence = -1;
};
