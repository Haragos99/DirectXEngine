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
	bool rKeyLatch = false;
	bool gizmoDragging = false; // true while dragging a gizmo axis handle
	bool leftWasDown = false;   // previous-frame left mouse button state
	void Render();
	float calculateDeltaTime();
	void RenderUI();
	void ProcessInput();
	void UpdateSelection();
	void HandleGizmoDrag();
	void Import(const std::wstring& path);
};