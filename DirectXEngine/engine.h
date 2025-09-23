#pragma once
#include "windowapp.h"
#include "graphics.h"
class Engine : public WindowApp {

public:
	Engine(HINSTANCE hInstance, int nCmdShow);
	int Run();
private:
	Graphics graphics;
	
};