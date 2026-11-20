#pragma once
// Precompiled header for the engine projects.
// It is force-included into every engine .cpp by Projects/Engine.props (/FI),
// so source files do not need to include it themselves.
// Only put headers here that rarely change: editing this file rebuilds everything.

#ifndef NOMINMAX
#define NOMINMAX // keep windows.h from defining min/max macros that break std::min/std::max
#endif

#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
