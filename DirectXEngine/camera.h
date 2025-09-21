#pragma once
#include <DirectXMath.h>

class Camera
{
public:
    Camera(float aspectRatio);

    void SetPosition(float x, float y, float z);
    void Move(float dx, float dy, float dz);
    void Rotate(float _pitch, float _yaw);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix() const;

private:
    DirectX::XMVECTOR position;
    float pitch;  // rotation around X
    float yaw;    // rotation around Y
    float roll;   // rotation around Z
    DirectX::XMMATRIX projection;
    float fov, aspect, nearZ, farZ;

    DirectX::XMFLOAT3 eye;     // Camera position
    DirectX::XMFLOAT3 lookAt;  // Target
    DirectX::XMFLOAT3 up;      // World up
};