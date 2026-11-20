#pragma once
#include <windows.h>
#include <string>

// Wraps the Win32 open-file dialog for model files. The filter is derived from
// the registered model loaders, so it lists every supported format without the
// UI knowing about any of them.
class ModelFileDialog
{
public:
	void SetOwner(HWND hwnd) { owner = hwnd; }

	// Chosen path, or an empty string when the user cancelled.
	std::wstring Show() const;

private:
	HWND owner = nullptr;
};
