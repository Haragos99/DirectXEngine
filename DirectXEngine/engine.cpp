#include "engine.h"


Engine::Engine(HINSTANCE hInstance, int nCmdShow) : WindowApp(hInstance, L"DirectX Engine Window", L"DirectX Engine Class", 1280, 720, nCmdShow), graphics(GetHWND(), 1280, 720)
{

}




int Engine::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
			graphics.Clear(0.0f, 0.2f, 0.4f, 1.0f);
			graphics.RenderFrame();
        }
    }
    return static_cast<int>(msg.wParam);
}