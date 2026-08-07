#include "mouse.h"
#include <windows.h>
void Mouse::OnMouseMove(int newX, int newY)
{
    prevX = x;
    prevY = y;
    x = newX;
    y = newY;
}

int Mouse::GetDeltaX() { return x - prevX; }
int Mouse::GetDeltaY() { return y - prevY; }

void Mouse::OnButtonDown(int button)
{
    if (button == VK_LBUTTON)
    {
        leftDown = true;
        leftClicked = true;
        clickX = x;
        clickY = y;
    }
    if (button == VK_RBUTTON)
    {
        rightDown = true;
    }
}

void Mouse::OnButtonUp(int button)
{
    if (button == VK_LBUTTON)
    {
        leftDown = false;
    }
    if (button == VK_RBUTTON)
    {
        rightDown = false;
    }
}

void Mouse::OnWheelDelta(int delta)
{
    wheelDelta += delta / 100;
}

int Mouse::GetWheelDelta()
{
    int delta = wheelDelta;
    wheelDelta = 0;
    return delta;
}

bool Mouse::IsButtonDown(int button) const
{
    if (button == VK_LBUTTON)
    {
        return leftDown;
    }
    if (button == VK_RBUTTON)
    {
        return rightDown;
    }
    return false;
}

void Mouse::OnLeftDoubleClick(int newX, int newY)
{
    leftDoubleClicked = true;
    doubleClickX = newX;
    doubleClickY = newY;
}

bool Mouse::ConsumeLeftClick(int& outX, int& outY)
{
    if (!leftClicked)
        return false;
    leftClicked = false;
    outX = clickX;
    outY = clickY;
    return true;
}

bool Mouse::ConsumeLeftDoubleClick(int& outX, int& outY)
{
    if (!leftDoubleClicked)
        return false;
    leftDoubleClicked = false;
    outX = doubleClickX;
    outY = doubleClickY;
    return true;
}