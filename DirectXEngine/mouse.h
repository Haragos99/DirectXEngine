#pragma once

class Mouse
{
public:
    Mouse() = default;
    void OnMouseMove(int x, int y);
    void OnButtonDown(int button);
    void OnButtonUp(int button);

    int GetX() const { return x; }
    int GetY() const { return y; }
    int GetDeltaX();
    int GetDeltaY();

    bool IsButtonDown(int button) const;
    ~Mouse() = default;
private:
    int x = 0, y = 0;
    int prevX = 0, prevY = 0;
    bool leftDown = false;
    bool rightDown = false;
};