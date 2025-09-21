#include "windowapp.h"

std::unique_ptr<Mouse> WindowApp::mouseEvent = nullptr;
std::unique_ptr<Keyboard> WindowApp::keyboardEvent = nullptr;


WindowApp::WindowApp(HINSTANCE hInstance, LPCWSTR window_title, LPCWSTR window_class, int width, int height, int nCmdShow) : windowHeight(height), windowWidth(width)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = window_class;
    wc.lpszMenuName = window_title;
    RegisterClass(&wc);

    keyboardEvent = std::make_unique<Keyboard>();
    mouseEvent = std::make_unique<Mouse>();

    hWnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        wc.lpszMenuName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    ShowWindow(hWnd, nCmdShow);
}


LRESULT CALLBACK WindowApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int x;
    int y;
    float cX;
    float cY;
    switch (msg)
    {
    case WM_KEYDOWN:
        keyboardEvent->OnKeyDown(static_cast<unsigned char>(wParam));
        return 0;
    case WM_KEYUP:
        keyboardEvent->OnKeyUp(static_cast<unsigned char>(wParam));
        return 0;
    case WM_MOUSEMOVE:
        x = LOWORD(lParam);
        y = HIWORD(lParam);

        // Convert to normalized device space
        cX = 2.0f * x / 1280 - 1;	// flip y axis
        cY = 1.0f - 2.0f * y / 720;
        mouseEvent->OnMouseMove(x, y);
        return 0;
    case WM_LBUTTONDOWN:
        mouseEvent->OnButtonDown(VK_LBUTTON);
        return 0;
    case WM_LBUTTONUP:
        mouseEvent->OnButtonUp(VK_LBUTTON);
        return 0;
    case WM_RBUTTONDOWN:
        mouseEvent->OnButtonDown(VK_RBUTTON);
        return 0;
    case WM_RBUTTONUP:
        mouseEvent->OnButtonUp(VK_RBUTTON);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

