#include "windowapp.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

// Provided by imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


std::unique_ptr<Mouse> WindowApp::mouseEvent = nullptr;
std::unique_ptr<Keyboard> WindowApp::keyboardEvent = nullptr;
WindowApp* WindowApp::instance = nullptr;


WindowApp::WindowApp(HINSTANCE hInstance, LPCWSTR window_title, LPCWSTR window_class, int width, int height, int nCmdShow) : windowHeight(height), windowWidth(width)
{
    instance = this;

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
    // Let ImGui inspect the message first (mouse/keyboard capture, etc.)
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    // If ImGui wants exclusive input, don't forward it to the engine.
    ImGuiIO* io = ImGui::GetCurrentContext() ? &ImGui::GetIO() : nullptr;
    const bool imguiWantsMouse    = io && io->WantCaptureMouse;
    const bool imguiWantsKeyboard = io && io->WantCaptureKeyboard;

    int x;
    int y;
    float cX;
    float cY;
    switch (msg)
    {
    case WM_KEYDOWN:
        if (imguiWantsKeyboard) return 0;
        keyboardEvent->OnKeyDown(static_cast<unsigned char>(wParam));
        return 0;
    case WM_KEYUP:
        if (imguiWantsKeyboard) return 0;
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
    case WM_MOUSEWHEEL:
        if (imguiWantsMouse) return 0;
        mouseEvent->OnWheelDelta(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_LBUTTONDOWN:
        if (imguiWantsMouse) return 0;
        mouseEvent->OnButtonDown(VK_LBUTTON);
        return 0;
    case WM_LBUTTONUP:
        mouseEvent->OnButtonUp(VK_LBUTTON);
        return 0;
    case WM_RBUTTONDOWN:
        if (imguiWantsMouse) return 0;
        mouseEvent->OnButtonDown(VK_RBUTTON);
        return 0;
    case WM_RBUTTONUP:
        mouseEvent->OnButtonUp(VK_RBUTTON);
        return 0;
    case WM_SIZE:
        if (instance && wParam != SIZE_MINIMIZED)
        {
            const int newW = LOWORD(lParam);
            const int newH = HIWORD(lParam);
            if (newW > 0 && newH > 0)
            {
                instance->windowWidth = newW;
                instance->windowHeight = newH;
                instance->OnResize(newW, newH);
            }
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

