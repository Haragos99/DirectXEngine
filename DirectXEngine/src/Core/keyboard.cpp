#include "Keyboard.h"

void Keyboard::OnKeyDown(unsigned char key) { keyStates[key] = true; }
void Keyboard::OnKeyUp(unsigned char key) { keyStates[key] = false; }

bool Keyboard::IsKeyDown(unsigned char key) const
{
    auto it = keyStates.find(key);
    return (it != keyStates.end() && it->second);
}
