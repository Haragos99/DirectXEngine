#include "camera.h"
using namespace DirectX;

Camera::Camera(float aspectRatio)
	: position(XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f)),
	pitch(0.0f),
	yaw(XM_PI),
	roll(0.0f),
	fov(XM_PIDIV4), aspect(aspectRatio), nearZ(0.1f), farZ(100.0f), eye(0.0f, 0.0f, -5.0f), lookAt(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f)
{
	// yaw/pitch describe the orbit direction from the target to the eye.
	// Initialized so the starting eye (0,0,-5) matches the stored angles.
}

void Camera::SetPosition(float x, float y, float z)
{
	position = XMVectorSet(x, y, z, 1.0f);
}

void Camera::Move(float dx, float dy, float dz)
{
	float speed = 0.5f;

	XMVECTOR eyeVec = XMLoadFloat3(&eye);
	XMVECTOR lookAtVec = XMLoadFloat3(&lookAt);
	XMVECTOR upVec = XMLoadFloat3(&up);

	// Camera basis: forward along the view, right on the horizon plane.
	XMVECTOR forward = XMVector3Normalize(lookAtVec - eyeVec);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(upVec, forward));

	// dz = forward/back, dx = strafe, dy = straight up/down (world up).
	XMVECTOR moveVec =
		forward * dz +
		right * dx +
		upVec * dy;

	moveVec = XMVectorScale(moveVec, speed);

	// Pan both eye and target together so the view direction stays fixed.
	eyeVec += moveVec;
	lookAtVec += moveVec;

	XMStoreFloat3(&eye, eyeVec);
	XMStoreFloat3(&lookAt, lookAtVec);
}

void Camera::Rotate(float _pitch, float _yaw)
{
	XMVECTOR wEye = XMLoadFloat3(&eye);
	XMVECTOR wLookat = XMLoadFloat3(&lookAt);

	// Preserve the current orbit distance so rotation never changes the zoom.
	float radius = XMVectorGetX(XMVector3Length(wEye - wLookat));
	if (radius < 0.001f)
		radius = 0.001f;

	// Accumulate the drag into the orbit angles.
	yaw += _yaw;
	pitch += _pitch;

	// Keep yaw in [0, 2pi) to avoid float drift over long sessions.
	yaw = fmodf(yaw, XM_2PI);
	if (yaw < 0.0f)
		yaw += XM_2PI;

	// Clamp pitch just short of the poles so the view can't flip upside down.
	const float pitchLimit = XM_PIDIV2 - 0.01f;
	if (pitch > pitchLimit)  pitch = pitchLimit;
	if (pitch < -pitchLimit) pitch = -pitchLimit;

	// Spherical direction from the target to the eye.
	const float cosPitch = cosf(pitch);
	XMVECTOR dir = XMVectorSet(
		cosPitch * sinf(yaw),
		sinf(pitch),
		cosPitch * cosf(yaw),
		0.0f);

	wEye = wLookat + dir * radius;
	XMStoreFloat3(&eye, wEye);
}

void Camera::Zoom(float wheelDelta)
{
	// wheelDelta comes in multiples of 10 from WM_MOUSEWHEEL.
	const float steps = wheelDelta / 10.0f;

	XMVECTOR eyeVec = XMLoadFloat3(&eye);
	XMVECTOR lookAtVec = XMLoadFloat3(&lookAt);

	XMVECTOR toEye = eyeVec - lookAtVec;
	float distance = XMVectorGetX(XMVector3Length(toEye));
	if (distance < 0.0001f)
		return;

	XMVECTOR dir = toEye / distance;

	// Multiplicative zoom: each step scales the distance for a smooth,
	// speed-scaled feel like most 3D software (zoom to the target).
	distance *= powf(0.9f, steps);

	// Clamp so the eye can't pass through or fly away from the target.
	const float minDistance = 0.2f;
	const float maxDistance = 500.0f;
	if (distance < minDistance) distance = minDistance;
	if (distance > maxDistance) distance = maxDistance;

	eyeVec = lookAtVec + dir * distance;
	XMStoreFloat3(&eye, eyeVec);
}

void Camera::SetAspect(float aspectRatio)
{
	if (aspectRatio > 0.0f)
		aspect = aspectRatio;
}

XMMATRIX Camera::GetViewMatrix() const
{
	return XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&lookAt), XMLoadFloat3(&up));
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);
}

Ray Camera::ScreenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight) const
{
	// 1) Pixel -> Normalized Device Coordinates ([-1, 1], y up).
	const float ndcX = (2.0f * screenX) / screenWidth - 1.0f;
	const float ndcY = 1.0f - (2.0f * screenY) / screenHeight;

	// 2) Unproject through the inverse view-projection to reach world space.
	const XMMATRIX invViewProj = XMMatrixInverse(nullptr, GetViewMatrix() * GetProjectionMatrix());

	// Near plane (z = 0) and far plane (z = 1) points in clip space.
	const XMVECTOR nearPoint = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invViewProj);
	const XMVECTOR farPoint  = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invViewProj);

	// 3) The ray goes from the near point towards the far point.
	const XMVECTOR direction = XMVector3Normalize(XMVectorSubtract(farPoint, nearPoint));

	Ray ray;
	XMStoreFloat3(&ray.origin, nearPoint);
	XMStoreFloat3(&ray.direction, direction);
	return ray;
}
