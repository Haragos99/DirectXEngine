#include "engine.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    Engine engine(hInstance, nCmdShow);
    return engine.Run();
}

