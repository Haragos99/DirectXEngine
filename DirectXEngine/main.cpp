#include "engine.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    Engine engine = Engine(hInstance, nCmdShow);
    return engine.Run();
}

