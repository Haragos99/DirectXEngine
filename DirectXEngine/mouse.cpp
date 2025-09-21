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