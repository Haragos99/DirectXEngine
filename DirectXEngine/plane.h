#pragma once
#include "object3d.h"


class Plane : public Object3D
{
public:
	Plane() = default;
	Plane(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex);
	~Plane() = default;
	void Draw(Camera camera) override;
	void Update(float time) override;
	void createTexturedVertex() override;
	void createIndeces() override;
private:
	std::unique_ptr<Texture> normalMap;
};
