#pragma once
#include <d3dcommon.h>
#include <wrl/client.h>
#include <filesystem>
#include <string>

// Finds the files the engine loads at runtime, independent of the working directory.
//   Shaders:   <exe dir>\shaders\<name>.cso   (compiled by the build, see FxCompile in the exe project)
//   Resources: the first "Resources" folder found walking up from the exe dir
//              (x64\Debug\DirectXEngine.exe -> <repo>\Resources), so the folder is never copied.
namespace AssetPaths
{
    const std::filesystem::path& ExeDir();
    const std::filesystem::path& ResourceDir();

    // Full path of a file under Resources, e.g. Resource(L"Models\\teapot.obj").
    std::wstring Resource(const std::wstring& relativePath);
    std::string ResourceA(const std::wstring& relativePath);

    // Compiled shader for a shader name. Objects keep identifying shaders by their
    // source name ("shaders\\MeshPixelShader.hlsl"); only the file stem is used, so this
    // maps to <exe dir>\shaders\MeshPixelShader.cso.
    std::wstring CompiledShader(const std::wstring& shaderName);

    // Reads the compiled shader for `shaderName`. Returns null and writes the missing
    // path to the debugger output when it cannot be read.
    Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const std::wstring& shaderName);
    // Like LoadShader, but throws std::runtime_error naming the missing .cso.
    Microsoft::WRL::ComPtr<ID3DBlob> RequireShader(const std::wstring& shaderName);
}
