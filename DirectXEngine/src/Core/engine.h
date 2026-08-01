#pragma once
#include "windowapp.h"
#include "graphics.h"
#include <chrono>
class Engine : public WindowApp {

public:
	Engine(HINSTANCE hInstance, int nCmdShow);

	~Engine() = default;
	int Run();
	void OnResize(int width, int height) override;
private:
	Graphics graphics;
	UIPanel ui;
	void Render();
	float calculateDeltaTime();
	void RenderUI();
	void Import(const std::wstring& path);
};