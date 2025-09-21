#include "camera.h"
using namespace DirectX;

Camera::Camera(float aspectRatio)
	: position(XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f)),
	pitch(0.0f),
	yaw(0.0f),
	roll(0.0f),
	fov(XM_PIDIV4), aspect(aspectRatio), nearZ(0.1f), farZ(100.0f), eye(0.0f, 0.0f, -5.0f), lookAt(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f)
{

}

void Camera::SetPosition(float x, float y, float z)
{
	position = XMVectorSet(x, y, z, 1.0f);
}

void Camera::Move(float dx, float dy, float dz)
{
	float speed = 0.5f;


	// Load vectors
	XMVECTOR eyeVec = XMLoadFloat3(&eye);
	XMVECTOR lookAtVec = XMLoadFloat3(&lookAt);
	XMVECTOR upVec = XMLoadFloat3(&up);

	// Forward, right, up vectors
	XMVECTOR forward = XMVector3Normalize(lookAtVec - eyeVec);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(upVec, forward));

	// Movement direction
	XMVECTOR moveVec =
		forward * dz +
		right * dx +
		upVec * dy;

	moveVec = XMVectorScale(moveVec, speed);

	// Apply movement to both eye and lookAt
	eyeVec += moveVec;
	lookAtVec += moveVec;

	XMStoreFloat3(&eye, eyeVec);
	XMStoreFloat3(&lookAt, lookAtVec);
}

void Camera::Rotate(float _pitch, float _yaw)
{
	// Convert to vectors
	XMVECTOR wEye = XMLoadFloat3(&eye);
	XMVECTOR wLookat = XMLoadFloat3(&lookAt);
	XMVECTOR wVup = XMLoadFloat3(&up);

	// View direction and right axis
	XMVECTOR w = XMVector3Normalize(wEye - wLookat);
	XMVECTOR u = XMVector3Normalize(XMVector3Cross(wVup, w));

	// Build rotation matrices
	XMMATRIX rotX = XMMatrixRotationAxis(u, _pitch);   // Horizontal
	XMMATRIX rotY = XMMatrixRotationAxis(wVup, _yaw);// Vertical

	// Local eye vector
	XMVECTOR lEye = wEye - wLookat;

	// Rotate around Y (safe)
	lEye = XMVector3Transform(lEye, rotY);
	wEye = lEye + wLookat;

	// Rotate around X (check gimbal lock)
	XMVECTOR lEyeTest = XMVector3Transform(lEye, rotX);

	// Dot product check (avoid looking straight up/down)
	XMVECTOR lEyeNorm = XMVector3Normalize(lEyeTest);
	float dotUp = XMVectorGetX(XMVector3Dot(lEyeNorm, XMVector3Normalize(wVup)));

	if (fabsf(dotUp) < 0.95f) {
		lEye = lEyeTest;
		wEye = lEye + wLookat;
	}

	XMStoreFloat3(&eye, wEye);
}

XMMATRIX Camera::GetViewMatrix() const
{
	return XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&lookAt), XMLoadFloat3(&up));
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);
}
