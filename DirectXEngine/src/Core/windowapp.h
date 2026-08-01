#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include "mouse.h"
#include "keyboard.h"
class WindowApp{
public:
    WindowApp(HINSTANCE hInstance, LPCWSTR window_title, LPCWSTR window_class, int width, int height, int nCmdShow);
	HWND GetHWND() const { return hWnd; }
    int GetWidth() const { return windowWidth; }
    int GetHeight() const { return windowHeight; }
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void ProcessMessages();
    static std::unique_ptr<Mouse> mouseEvent;
    static std::unique_ptr<Keyboard> keyboardEvent;
    // Called from WM_SIZE. Override in derived classes to react to window resizes.
    virtual void OnResize(int width, int height) {}
protected:
    int windowWidth;
    int windowHeight;
private:
    static WindowApp* instance;
    HINSTANCE hInst;
    HWND hWnd;
};