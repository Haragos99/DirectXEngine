#include "engine.h"


Engine::Engine(HINSTANCE hInstance, int nCmdShow) : WindowApp(hInstance, L"DirectX Engine Window", L"DirectX Engine Class", 1280, 720, nCmdShow), graphics(GetHWND(), 1280, 720)
{
	ui.Init(GetHWND(), graphics.GetDevice(), graphics.GetContext());
	ui.SetImportModelCallback([this](const std::wstring& path) {
		Import(path);
	});
}

int Engine::Run()
{
	MSG msg = {};
	bool rKeyLatch = false;
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			const float* clearColor = ui.GetClearColor();
			graphics.Clear(clearColor[0], clearColor[1], clearColor[2], 1.0f);
			float speed = 0.05f;
			if (keyboardEvent->IsKeyDown('W')) 
			{ 
				graphics.camera.Move(0, speed, 0);
			}
			if (keyboardEvent->IsKeyDown('S')) 
			{ 
				graphics.camera.Move(0, -speed, 0);
			}
			if (keyboardEvent->IsKeyDown('A')) 
			{ 
				graphics.camera.Move(-speed, 0, 0);
			}
			if (keyboardEvent->IsKeyDown('D'))
			{
				graphics.camera.Move(speed, 0, 0);
			}
			if (keyboardEvent->IsKeyDown(VK_SPACE))
			{
				graphics.camera.Move(0, speed, 0);
			}
			if (keyboardEvent->IsKeyDown(VK_SHIFT))
			{
				graphics.camera.Move(0, -speed, 0);
			}

			const bool rDown = keyboardEvent->IsKeyDown('R');
			if (rDown && !rKeyLatch)
			{
				graphics.changeRenderMode();
			}
			rKeyLatch = rDown;

			if (mouseEvent->IsButtonDown(VK_LBUTTON))
			{
				int deltax = mouseEvent->GetDeltaX();
				int deltay = mouseEvent->GetDeltaY();
				float x = (2.0f * deltax) / windowHeight - 1.0f;
				float y = 1.0f - (2.0f * deltay) / windowWidth;
				graphics.camera.Rotate(deltay * 0.005, deltax * 0.005);
			}

			int wheelDelta = mouseEvent->GetWheelDelta();
			if (wheelDelta != 0)
			{
				graphics.camera.Move(0, 0, wheelDelta);
			}
			
			

			float dt = calculateDeltaTime();
			graphics.Update(dt);
			Render();
		}
	}
	return static_cast<int>(msg.wParam);
}

void Engine::Render()
{
	graphics.RenderFrame();
	RenderUI();
	graphics.swapChainPresent();
}


void Engine::RenderUI()
{
	// ImGui overlay
	ui.BeginFrame();
	ui.DrawTestPanel();
	if (ui.RequestedRenderModeChange())
	{
		graphics.changeRenderMode();
	}
	ui.EndFrame();
}

float Engine::calculateDeltaTime()
{
	static auto previousTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> elapsed = currentTime - previousTime;

	previousTime = currentTime;

	return elapsed.count(); // seconds
}

void Engine::OnResize(int width, int height)
{
	windowHeight = height;
	windowWidth = width;
	graphics.Resize(static_cast<UINT>(width), static_cast<UINT>(height));
}

void Engine::Import(const std::wstring& path)
{
	graphics.ImportModel(path);
}