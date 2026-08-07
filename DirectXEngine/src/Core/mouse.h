#pragma once

class Mouse
{
public:
    Mouse() = default;
    void OnMouseMove(int x, int y);
    void OnButtonDown(int button);
    void OnButtonUp(int button);
    void OnWheelDelta(int delta);
    void OnLeftDoubleClick(int x, int y);

    int GetX() const { return x; }
    int GetY() const { return y; }
    int GetDeltaX();
    int GetDeltaY();
    int GetWheelDelta();

    bool IsButtonDown(int button) const;

    // One-shot click queries. Each returns true only once per event and writes the
    // pixel position of the click to (outX, outY).
    bool ConsumeLeftClick(int& outX, int& outY);
    bool ConsumeLeftDoubleClick(int& outX, int& outY);
    ~Mouse() = default;
private:
    int x = 0, y = 0;
    int prevX = 0, prevY = 0;
    int wheelDelta = 0;
    bool leftDown = false;
    bool rightDown = false;
    bool leftClicked = false;
    bool leftDoubleClicked = false;
    int clickX = 0, clickY = 0;
    int doubleClickX = 0, doubleClickY = 0;
};