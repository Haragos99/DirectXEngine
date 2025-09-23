#pragma once
#include "object3d.h"



struct ModelViewProjectionBuffer
{
    DirectX::XMMATRIX modelViewProjection;
};
class EnvCube 
{
public:
    EnvCube() = default;
    EnvCube(Microsoft::WRL::ComPtr<ID3D11Device> gfx, Microsoft::WRL::ComPtr<ID3D11DeviceContext> fx);

    void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> gfx, Camera camera);

private:
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMapSRV;
    Microsoft::WRL::ComPtr<ID3D11Buffer> matrixBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> raster;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_DepthStencilState;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
};