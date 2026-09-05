#pragma once
#include "imodelloader.h"
#include "object3d.h"

#include <string>

// Scene object for a mesh that was generated rather than loaded, such as the
// surface marching cubes extracts from a voxel grid. It takes the geometry
// ready made, so the object never learns where it came from.
class IsoSurfaceModel : public Object3D
{
public:
	IsoSurfaceModel(MeshData meshData,
	                std::string modelName,
	                const std::wstring& VSPath,
	                const std::wstring& PSPath,
	                Microsoft::WRL::ComPtr<ID3D11Device> _device,
	                Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);

	void Update(float time) override;

	size_t GetTriangleCount() const { return data.indices.size() / 3; }

protected:
	void createTexturedVertex() override;
	void createIndeces() override;

private:
	MeshData data;
};
