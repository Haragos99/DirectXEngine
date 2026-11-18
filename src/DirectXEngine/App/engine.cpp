#include "engine.h"
#include "ivoxelsource.h"
#include <cstdio>
#include <cmath>


Engine::Engine(HINSTANCE hInstance, int nCmdShow) : WindowApp(hInstance, L"DirectX Engine Window", L"DirectX Engine Class", 1280, 720, nCmdShow), renderer(GetHWND(), 1280, 720),
	world(renderer.Device(), renderer.Context(), 1280.0f / 720.0f),
	editor(world, renderer.Device(), renderer.Context())
{
	ui.Init(GetHWND(), renderer.GetDevice(), renderer.GetContext());
	ui.SetImportModelCallback([this](const std::wstring& path) {
		Import(path);
	});
	ui.SetCreateSkeletonCallback([this]() {
		editor.AddSkeleton();
	});
	ui.SetCreateIsoSurfaceCallback([this](std::shared_ptr<IVoxelSource> volume, int resolution, float isoLevel) {
		editor.AddIsoSurface(volume, resolution, isoLevel);
	});
	ui.SetReparentCallback([this](std::shared_ptr<Object3D> child, std::shared_ptr<Object3D> newParent) {
		world.Reparent(child, newParent);
	});
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
			const float* clearColor = ui.GetClearColor();
			renderer.Clear(clearColor[0], clearColor[1], clearColor[2], 1.0f);

			const float dt = calculateDeltaTime();
			HandleGizmoDrag();
			ProcessInput();
			UpdateSelection();
			editor.Update(dt);
			Render();
		}
	}
	return static_cast<int>(msg.wParam);
}

void Engine::ProcessInput()
{
	const float speed = 0.05f;
	if (keyboardEvent->IsKeyDown('W'))     
	{ 
		world.camera.Move(0, 0, speed); // forward
	}   
	if (keyboardEvent->IsKeyDown('S'))     
	{ 
		world.camera.Move(0, 0, -speed); // back
	}  
	if (keyboardEvent->IsKeyDown('A'))     
	{ 
		world.camera.Move(-speed, 0, 0); // strafe left
	}  
	if (keyboardEvent->IsKeyDown('D'))    
	{ 
		world.camera.Move(speed, 0, 0); // strafe right
	}  
	if (keyboardEvent->IsKeyDown(VK_SPACE)) 
	{ 
		world.camera.Move(0, speed, 0); // up
	}  
	if (keyboardEvent->IsKeyDown(VK_SHIFT)) 
	{ 
		world.camera.Move(0, -speed, 0); // down
	}
	if(keyboardEvent->IsKeyDown(VK_DELETE))
	{
		RemoveSelectedObject();
	}

	const bool rDown = keyboardEvent->IsKeyDown('R');
	if (rDown && !rKeyLatch)
		world.CycleRenderMode();
	rKeyLatch = rDown;

	// Drag with the left button to orbit the camera (unless a gizmo axis is
	// being dragged, which takes priority over camera movement).
	if (!gizmoDragging && (mouseEvent->IsButtonDown(VK_LBUTTON) && keyboardEvent->IsKeyDown(VK_CONTROL)))
	{
		const int deltax = mouseEvent->GetDeltaX();
		const int deltay = mouseEvent->GetDeltaY();
		world.camera.Rotate(deltay * 0.005f, deltax * 0.005f);
	}

	const int wheelDelta = mouseEvent->GetWheelDelta();
	if (wheelDelta != 0)
		world.camera.Zoom(static_cast<float>(wheelDelta));
}

void Engine::Render()
{
	renderer.BeginFrame();
	world.Draw(renderer);
	editor.DrawOverlay();
	RenderUI();
	renderer.Present();
}

void Engine::RenderUI()
{
	// ImGui overlay
	ui.BeginFrame();
	ui.SetMeshingStatus(editor.IsMeshing(), editor.GetMeshingProgress(), editor.GetMeshingLabel());
	ui.DrawTestPanel(world.Objects());
	if (ui.RequestedRenderModeChange())
	{
		world.CycleRenderMode();
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
	renderer.Resize(static_cast<UINT>(width), static_cast<UINT>(height));
	if (width > 0 && height > 0)
		world.camera.SetAspect(static_cast<float>(width) / static_cast<float>(height));
}

void Engine::UpdateSelection()
{
	int clickX = 0;
	int clickY = 0;

	// Single left click: report the click position in screen and world space.
	if (mouseEvent->ConsumeLeftClick(clickX, clickY))
	{
		const Ray ray = world.camera.ScreenPointToRay(
			static_cast<float>(clickX), static_cast<float>(clickY),
			static_cast<float>(windowWidth), static_cast<float>(windowHeight));

		DirectX::XMFLOAT3 hitPoint{};
		const bool onObject = world.PickObject(ray, &hitPoint) != nullptr;

		if (!onObject)
		{
			// Nothing hit: fall back to where the ray meets the ground plane (y = 0).
			if (fabsf(ray.direction.y) > 1e-6f)
			{
				const float t = -ray.origin.y / ray.direction.y;
				hitPoint.x = ray.origin.x + ray.direction.x * t;
				hitPoint.y = 0.0f;
				hitPoint.z = ray.origin.z + ray.direction.z * t;
			}
			else
			{
				hitPoint = ray.origin;
			}
		}

		char buffer[256];
		sprintf_s(buffer, "[Click] screen=(%d, %d)  world=(%.3f, %.3f, %.3f)%s\n",
			clickX, clickY, hitPoint.x, hitPoint.y, hitPoint.z, onObject ? "  (on object)" : "");
		OutputDebugStringA(buffer);
	}

	// Double left click: pick the object under the cursor and make it the selection.
	if (mouseEvent->ConsumeLeftDoubleClick(clickX, clickY))
	{
		const Ray ray = world.camera.ScreenPointToRay(
			static_cast<float>(clickX), static_cast<float>(clickY),
			static_cast<float>(windowWidth), static_cast<float>(windowHeight));

		ui.SetSelectedObject(world.PickObject(ray, nullptr));
	}

	// Keep the gizmo attached to whatever is selected (via double-click or the UI panel).
	editor.SetSelectedObject(ui.GetSelectedObject());
}

void Engine::HandleGizmoDrag()
{
	const bool leftDown = mouseEvent->IsButtonDown(VK_LBUTTON);

	// Apply the transform mode (Move/Scale) chosen in the UI panel.
	editor.SetGizmoMode(ui.GetGizmoMode());

	if (!editor.Gizmo() || !editor.Gizmo()->IsVisible())
	{
		if (gizmoDragging)
			editor.Gizmo()->EndDrag();
		gizmoDragging = false;
		leftWasDown = leftDown;
		return;
	}

	const Ray ray = world.camera.ScreenPointToRay(
		static_cast<float>(mouseEvent->GetX()), static_cast<float>(mouseEvent->GetY()),
		static_cast<float>(windowWidth), static_cast<float>(windowHeight));

	if (leftDown && !leftWasDown)
	{
		// Button just pressed: grab the axis handle under the cursor, if any.
		const GizmoController::Axis axis = editor.Gizmo()->PickAxis(ray);
		if (axis != GizmoController::Axis::None)
		{
			editor.Gizmo()->BeginDrag(axis, ray);
			gizmoDragging = true;
		}
	}
	else if (leftDown && gizmoDragging)
	{
		editor.Gizmo()->UpdateDrag(ray);
	}
	else if (!leftDown && gizmoDragging)
	{
		editor.Gizmo()->EndDrag();
		gizmoDragging = false;
	}

	leftWasDown = leftDown;
}

void Engine::Import(const std::wstring& path)
{
	editor.ImportModel(path);
}

void Engine::RemoveSelectedObject()
{
	if (const std::shared_ptr<Object3D> selected = ui.GetSelectedObject())
	{
		ui.SetSelectedObject(nullptr); // clear before the object goes away
		world.RemoveObject(selected);
	}
}