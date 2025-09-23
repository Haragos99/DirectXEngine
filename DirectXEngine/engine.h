#pragma once
#include "windowapp.h"
#include "graphics.h"
class Engine : public WindowApp {

public:
	Engine(HINSTANCE hInstance, int nCmdShow);

	~Engine() = default;
	int Run();
private:
	Graphics graphics;
	
};