#include "engine.h"


Engine::Engine(HINSTANCE hInstance, int nCmdShow) : WindowApp(hInstance, L"DirectX Engine Window", L"DirectX Engine Class", 1280, 720, nCmdShow), graphics(GetHWND(), 1280, 720)
{

}




int Engine::Run()
{
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{

			graphics.Clear(0.0f, 0.2f, 0.4f, 1.0f);
			float speed = 0.05f;
			if (keyboardEvent->IsKeyDown('W')) graphics.camera.Move(0, 0, speed);
			if (keyboardEvent->IsKeyDown('S')) graphics.camera.Move(0, 0, -speed);
			if (keyboardEvent->IsKeyDown('A')) graphics.camera.Move(-speed, 0, 0);
			if (keyboardEvent->IsKeyDown('D')) graphics.camera.Move(speed, 0, 0);
			if (keyboardEvent->IsKeyDown(VK_SPACE)) graphics.camera.Move(0, speed, 0);
			if (keyboardEvent->IsKeyDown(VK_SHIFT)) graphics.camera.Move(0, -speed, 0);


			if (mouseEvent->IsButtonDown(VK_LBUTTON))
			{
				int deltax = mouseEvent->GetDeltaX();
				int deltay = mouseEvent->GetDeltaY();
				graphics.camera.Rotate(deltax * 0.05, deltay * 0.05);
			}
			graphics.RenderFrame();
		}
	}
	return static_cast<int>(msg.wParam);
}