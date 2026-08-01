#pragma once
#include "shaders.h"

class RaytraceShader : public Shader
{
public:
	RaytraceShader(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext>  _context);
	void createCameraBuffer();
	void creatQuadricsBuffer();
	void LoadShaders(const std::wstring& vertexShaderPath, const std::wstring& pixelShaderPath) override;
	void renderRaytraceDraw(CameraCB& cb, SphereBuffer& sp);
	void createVertexRTBuffer(std::vector<VertexRT> vertices);
	~RaytraceShader() = default;
protected:
	void loadVertexShader(const std::wstring& vertexShaderPath) override;
	Microsoft::WRL::ComPtr<ID3D11Buffer> cameraBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> sphereCBuffer;

};