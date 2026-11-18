#pragma once
#include <DirectXMath.h>

// A world-space ray, used for mouse picking / object selection.
struct Ray
{
    DirectX::XMFLOAT3 origin;
    DirectX::XMFLOAT3 direction; // normalized
};

class Camera
{
public:
    Camera(float aspectRatio);

    void SetPosition(float x, float y, float z);
    void Move(float dx, float dy, float dz);
    void Rotate(float _pitch, float _yaw);
    void Zoom(float wheelDelta);
    void SetAspect(float aspectRatio);
    DirectX::XMFLOAT3 getPos() { return eye; }
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix() const;

    // Build a world-space picking ray from a pixel position in the viewport.
    // (screenX, screenY) are in pixels with the origin at the top-left corner.
    Ray ScreenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight) const;

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