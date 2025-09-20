#include "camera.h"
using namespace DirectX;

Camera::Camera(float aspectRatio)
    : position(XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f)),
    pitch(0.0f),
    yaw(0.0f),
	roll(0.0f)
{
    projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 100.0f);
}

void Camera::SetPosition(float x, float y, float z)
{
    position = XMVectorSet(x, y, z, 1.0f);
}

void Camera::Move(float dx, float dy, float dz)
{
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotation);
    XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotation);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    position += dx * right + dy * up + dz * forward;
}

void Camera::Rotate(float _pitch, float _yaw, float _rool)
{
    pitch += _pitch;
    yaw += _yaw;
	roll += _rool;
}

XMMATRIX Camera::GetViewMatrix() const
{
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    XMVECTOR target = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotation);
    XMVECTOR up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotation);

    return XMMatrixLookAtLH(position, position + target, up);
}

XMMATRIX Camera::GetProjectionMatrix() const
{
    return projection;
}
