#pragma once
#include "pointcloud.h"

#include <string>

namespace Ply
{
	// Reads a regular PLY point cloud: a vertex element with positions and,
	// optionally, per-point colours. Gaussian splat files are handled by
	// GaussianSplatPlyLoader instead.
	class PointCloudPlyLoader
	{
	public:
		// True when `path` is a PLY holding loose points - it has vertices but
		// no faces and none of the Gaussian splat attributes. Only the header
		// is read.
		static bool IsPointCloudFile(const std::string& path);

		bool Load(const std::string& path, PointCloud& out, std::string* outError = nullptr) const;
	};
}
