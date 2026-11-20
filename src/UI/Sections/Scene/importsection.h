#pragma once
#include "ipanelsection.h"
#include "modelfiledialog.h"

#include <functional>
#include <string>

// "Import model" button and the path of the last imported file.
class ImportSection : public IPanelSection
{
public:
	using ImportModelCallback = std::function<void(const std::wstring&)>;

	void SetOwnerWindow(HWND hwnd) { dialog.SetOwner(hwnd); }
	void SetCallback(ImportModelCallback callback) { importModel = std::move(callback); }

	const char* GetTitle() const override { return "Model"; }
	void Draw(UIState& state) override;

private:
	ModelFileDialog dialog;
	ImportModelCallback importModel;
	std::wstring lastImportedPath;
};
