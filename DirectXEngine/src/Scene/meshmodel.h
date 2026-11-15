#pragma once
#include "object3d.h"
#include "mesh.h"
class MeshModel : public Object3D
{
public:
	MeshModel() = default;
	MeshModel(std::string path, std::wstring VSPath, std::wstring PSPath,Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _contex);
	~MeshModel() = default;
	void Update(float time) override;
	void createTexturedVertex() override;
	void createIndeces() override;
	const std::vector<VertexData>* GetSkinVertices() const override { return &vertices; }
	const HalfEdgeMesh* GetHalfEdgeMesh() const override { return &mesh.GetHalfEdge(); }
	private:
		Mesh mesh;
};
