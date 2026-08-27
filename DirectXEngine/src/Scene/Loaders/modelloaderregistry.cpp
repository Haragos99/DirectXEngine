#include "modelloaderregistry.h"
#include "objmodelloader.h"
#include "plymodelloader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

ModelLoaderRegistry& ModelLoaderRegistry::Instance()
{
	static ModelLoaderRegistry registry = []
	{
		ModelLoaderRegistry created;
		created.Register(std::make_shared<ObjModelLoader>());
		created.Register(std::make_shared<PlyModelLoader>());
		return created;
	}();
	return registry;
}

void ModelLoaderRegistry::Register(std::shared_ptr<IModelLoader> loader)
{
	if (loader)
	{
		loaders.push_back(std::move(loader));
	}
}

std::string ModelLoaderRegistry::ExtensionOf(const std::string& path)
{
	std::string extension = std::filesystem::path(path).extension().string();
	if (!extension.empty() && extension.front() == '.')
	{
		extension.erase(extension.begin());
	}

	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return extension;
}

std::shared_ptr<IModelLoader> ModelLoaderRegistry::FindLoaderFor(const std::string& path) const
{
	const std::string extension = ExtensionOf(path);
	for (const auto& loader : loaders)
	{
		if (loader->CanLoad(extension))
		{
			return loader;
		}
	}
	return nullptr;
}

bool ModelLoaderRegistry::Load(const std::string& path, MeshData& out) const
{
	auto loader = FindLoaderFor(path);
	if (!loader)
	{
		return false;
	}

	return loader->Load(path, out);
}

std::wstring ModelLoaderRegistry::BuildFileDialogFilter() const
{
	// OPENFILENAME expects "Label\0pattern\0...\0\0".
	auto widen = [](const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	};

	std::wstring filter;
	std::wstring allPatterns;

	for (const auto& loader : loaders)
	{
		std::wstring patterns;
		for (const std::string& extension : loader->GetSupportedExtensions())
		{
			if (!patterns.empty()) 
			{ 
				patterns += L";";
			}

			patterns += L"*." + widen(extension);
		}
		if (patterns.empty())
		{
			continue;
		}

		if (!allPatterns.empty()) 
		{ 
			allPatterns += L";";
		}

		allPatterns += patterns;

		filter += widen(loader->GetFormatName()) + L" (" + patterns + L")";
		filter.push_back(L'\0');
		filter += patterns;
		filter.push_back(L'\0');
	}

	if (!allPatterns.empty())
	{
		filter = L"All supported models (" + allPatterns + L")" + std::wstring(1, L'\0')
			+ allPatterns + std::wstring(1, L'\0') + filter;
	}

	filter += L"All Files";
	filter.push_back(L'\0');
	filter += L"*.*";
	filter.push_back(L'\0');
	filter.push_back(L'\0');
	return filter;
}
