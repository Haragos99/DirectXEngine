#include "modelfiledialog.h"
#include "modelloaderregistry.h"

#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

std::wstring ModelFileDialog::Show() const
{
	if (!owner)
		return {};

	OPENFILENAMEW ofn = {};
	wchar_t fileBuffer[MAX_PATH] = L"";

	const std::wstring filter = ModelLoaderRegistry::Instance().BuildFileDialogFilter();

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = owner;
	ofn.lpstrFile = fileBuffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter.c_str();
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	return GetOpenFileNameW(&ofn) ? std::wstring(fileBuffer) : std::wstring();
}
