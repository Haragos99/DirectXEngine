#pragma once
#include "object3d.h"


class Cube : public Object3D
{
	public:
	Cube() = default;
	Cube(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex);
	~Cube() = default;
	void Draw(Camera camera) override;
	void Update(float time) override;
	void createTexturedVertex() override;
	void createIndeces() override;
};