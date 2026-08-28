#pragma once
#include <DirectXMath.h>

// Absolute translation/rotation/scale of an object. Keeping the components next
// to the world matrix instead of only inside it lets the UI read them back and
// edit them the way a 3D editor does.
struct Transform
{
	DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
	// Pitch, yaw and roll in radians.
	DirectX::XMFLOAT3 rotation{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };

	// Composes scale, then rotation, then translation, so scaling and rotating
	// always happen around the object's own origin.
	DirectX::XMMATRIX ToMatrix() const;
};
