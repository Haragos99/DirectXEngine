#pragma once
#include "shaders.h"

#include <cstdint>

// Rendering back end for 3D Gaussian Splatting.
//
// Splats are drawn as instanced screen aligned quads: a four vertex quad in slot 0
// is expanded per splat, and slot 1 carries the draw order - one index per
// instance into the splat structured buffer. Keeping the payload in a structured
// buffer means a depth sort only has to re-upload 4 bytes per splat.
// The vertex shader projects each 3D covariance into a 2D screen space conic and
// the pixel shader evaluates the Gaussian, so no textures are involved.
class GaussianSplatShader : public Shader
{
public:
	GaussianSplatShader(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	// Uploads the shared quad (slot 0) and its index buffer.
	void CreateQuadGeometry();
	// Uploads the splat cloud as the structured buffer the vertex shader reads.
	bool CreateSplatBuffer(const std::vector<SplatInstance>& splats);
	// Allocates the per-frame draw order stream (slot 1), one index per splat.
	bool CreateOrderBuffer(size_t splatCount);
	// Uploads a freshly sorted back-to-front draw order.
	void UpdateOrder(const std::vector<std::uint32_t>& order);
	void CreateFrameBuffer();
	// Alpha blending with depth testing but no depth writes, so splats never
	// occlude each other in the depth buffer.
	void CreateBlendState();

	// Binds the whole splat pipeline and issues one instanced draw call.
	void DrawSplats(const MatrixBuffer& matrices, const SplatFrameBuffer& frame, UINT instanceCount);
	// Restores the blend/depth state the rest of the scene expects.
	void RestoreDefaultStates();

protected:
	// Splat specific input layout (per-vertex corner + per-instance splat index).
	void loadVertexShader(const std::wstring& vertexShaderPath) override;

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> splatBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> splatSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> orderBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> frameBuffer;
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthReadOnlyState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> splatRS;
};
