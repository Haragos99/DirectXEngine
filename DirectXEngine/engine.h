#pragma once
#include "windowapp.h"
#include "graphics.h"
#include <chrono>
class Engine : public WindowApp {

public:
	Engine(HINSTANCE hInstance, int nCmdShow);

	~Engine() = default;
	int Run();
private:
	Graphics graphics;
	float calculateDeltaTime();
	
};