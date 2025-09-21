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
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void ProcessMessages();
    static std::unique_ptr<Mouse> mouseEvent;
    static std::unique_ptr<Keyboard> keyboardEvent;
private:
    int windowWidth;
    int windowHeight;
    HINSTANCE hInst;
    HWND hWnd;
};