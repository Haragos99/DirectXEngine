#pragma once
#include <DirectXMath.h>

class Camera
{
public:
    Camera(float aspectRatio);

    void SetPosition(float x, float y, float z);
    void Move(float dx, float dy, float dz);
    void Rotate(float _pitch, float _yaw, float _roll);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix() const;

private:
    DirectX::XMVECTOR position;
    float pitch;  // rotation around X
    float yaw;    // rotation around Y
    float roll;   // rotation around Z
    DirectX::XMMATRIX projection;
};