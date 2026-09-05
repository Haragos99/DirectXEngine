#pragma once
#include <functional>

// Fraction in [0, 1] reported by the long running voxel and meshing steps.
// It is invoked from whatever thread is doing the work, so an implementation
// must not touch the renderer or the UI directly.
using ProgressCallback = std::function<void(float)>;
