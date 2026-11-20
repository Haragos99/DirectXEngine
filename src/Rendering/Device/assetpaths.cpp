#include "assetpaths.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace fs = std::filesystem;

namespace AssetPaths
{
    const fs::path& ExeDir()
    {
        static const fs::path dir = []
        {
            std::wstring buffer(MAX_PATH, L'\0');
            DWORD length = 0;
            while ((length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()))) == buffer.size())
                buffer.resize(buffer.size() * 2);
            buffer.resize(length);
            return fs::path(buffer).parent_path();
        }();
        return dir;
    }

    const fs::path& ResourceDir()
    {
        static const fs::path dir = []
        {
            for (fs::path candidate = ExeDir(); !candidate.empty(); candidate = candidate.parent_path())
            {
                std::error_code ec;
                if (fs::is_directory(candidate / L"Resources", ec))
                    return candidate / L"Resources";
                if (candidate == candidate.root_path())
                    break;
            }
            OutputDebugStringW((L"[AssetPaths] No Resources folder found above " + ExeDir().wstring() + L"\n").c_str());
            return ExeDir() / L"Resources";
        }();
        return dir;
    }

    std::wstring Resource(const std::wstring& relativePath)
    {
        return (ResourceDir() / relativePath).wstring();
    }

    std::string ResourceA(const std::wstring& relativePath)
    {
        return (ResourceDir() / relativePath).string();
    }

    std::wstring CompiledShader(const std::wstring& shaderName)
    {
        return (ExeDir() / L"shaders" / fs::path(shaderName).stem()).wstring() + L".cso";
    }

    Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const std::wstring& shaderName)
    {
        const std::wstring path = CompiledShader(shaderName);
        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        if (FAILED(D3DReadFileToBlob(path.c_str(), &blob)))
        {
            OutputDebugStringW((L"[AssetPaths] Cannot read compiled shader " + path + L"\n").c_str());
            return nullptr;
        }
        return blob;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> RequireShader(const std::wstring& shaderName)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> blob = LoadShader(shaderName);
        if (!blob)
            throw std::runtime_error("Compiled shader not found: " + fs::path(CompiledShader(shaderName)).string());
        return blob;
    }
}
