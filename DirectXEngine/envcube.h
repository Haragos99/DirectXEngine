#pragma once
#include "object3d.h"
class EnvCube : public Object3D
{
public:
    EnvCube(Microsoft::WRL::ComPtr<ID3D11Device> gfx, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMapSRV);

    void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> gfx, Camera camera) override;

private:
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMapSRV;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cameraBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
   
};