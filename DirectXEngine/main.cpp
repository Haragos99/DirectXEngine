// main.cpp
// Minimal DirectX 11 initialization + rotating triangle (compile with Visual Studio)

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdint.h>
#include <assert.h>
#include "engine.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    Engine engine = Engine(hInstance, nCmdShow);
    return engine.Run();
}

