#pragma once
#include "object3d.h"
class Raytracer 
{
public:
    Raytracer(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

    void Draw(Camera camera, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMapSRV, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState);

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cameraBuffer;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Device> device; 
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMapSRV; 
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
};
