#pragma once
#include <unordered_map>

class Keyboard
{
public:
    Keyboard() = default;
    void OnKeyDown(unsigned char key);
    void OnKeyUp(unsigned char key);

    bool IsKeyDown(unsigned char key) const;
    ~Keyboard() = default;
private:
    std::unordered_map<unsigned char, bool> keyStates;
};