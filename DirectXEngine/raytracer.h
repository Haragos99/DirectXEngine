#pragma once
#include "object3d.h"
#include "raytraceshader.h"

struct SpherePhysics
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    float radius;
    float mass;
};


class Raytracer 
{
public:
    Raytracer(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
    SphereBuffer initElemt();
    void initSpheres();
    void Update(float time);
    void Draw(Camera camera);


private:
	void createIndeces();
    void resolveSphereCollisions();
    void resolveFloorCollision(SpherePhysics& s);
    
    void resolveCollision(SpherePhysics& A, SpherePhysics& B, DirectX::XMVECTOR diff, float dist, float minDist);
    std::vector<SpherePhysics> spheres;
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
    float floorHeight = -1.5f;
    float restitution = 0.8f;   // 1 = perfect bounce, <1 = energy loss
    float friction = 0.8f;      // horizontal damping

};
