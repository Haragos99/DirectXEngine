#pragma once
#include "object3d.h"
#include "raytraceshader.h"
class Raytracer 
{
public:
    Raytracer(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
    SphereBuffer initElemt();
    void Update(float time) ;
    void Draw(Camera camera) ;


private:
	void createIndeces();
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cameraBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> sphereCBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Device> device; 
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    std::unique_ptr <RaytraceShader> shaderRay;
    std::vector<UINT>   indices;

};
