// SampleApp.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SampleApp.h"

#define MAX_LOADSTRING 100

#define BUNNY 1
#define DRAGON 2
#define SPONZA 3
#define USED_SCENE SPONZA

#define BASIC 1
#define Debug_SGC 2
#define Debug_AB 3
#define Debug_EB 4
#define Debug_RT 5
#define USED_PROCESS Debug_RT

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
//HWND hWnd;										// the main window handle

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void StartTimming();
void Render();
float GetTimming();

// Rendering System
Presenter *presenter;
SScene *scene;
DrawSceneProcess *process;

// GUI parameters
bool ShowInfo = false;
ULONGLONG CurrentTime = 0;
bool TimeIsRunning = false;
float FPS;
ULONGLONG frameCount = 0;
float nextShow;

enum DEBUG_INFO_MODE {
	DEBUG_INFO_NONE,
	DEBUG_INFO_LAYER,
	DEBUG_INFO_FACE,
	DEBUG_INFO_LEVEL
};
DEBUG_INFO_MODE DebugInfoChanging = DEBUG_INFO_MODE::DEBUG_INFO_LAYER;

void InitializeScene(DeviceManager* manager)
{
	scene = new Scene<VERTEX, MATERIAL>(manager);
	// Camera setup
	scene->getCamera()->Position = float3(6, 3, 0);
	scene->getCamera()->Target = float3(0, 1, 0);
	scene->getCamera()->Up = float3(0, 1, 0);
	scene->getCamera()->FoV = PI / 2;
	scene->getCamera()->NearPlane = 0.1;
	scene->getCamera()->FarPlane = 1000;

	// Light setup
	scene->getLight()->Position = float3(0, 14, 0);
	scene->getLight()->Intensity = float3(100, 100, 100);

	// Back color
	scene->setBackColor(float4(0.2, 0.2, 0.4, 1));

	switch (USED_SCENE)
	{
	case BUNNY:
		scene->LoadObj(".\\Models\\bunny.obj", 10);
		break;
	case DRAGON:
		scene->LoadObj(".\\Models\\dragon.obj", 1);
		break;
	case SPONZA:
		scene->LoadObj(".\\Models\\sponza\\sponza.obj", 0.1f);
		break;
	default:
		break;
	}

	int matToMod = 17;//sponza floor
	//int matToMod = 0;

	if (scene->MaterialCount() > matToMod)
	{
		MATERIAL* m = scene->getMaterial(matToMod);
		//m->Diffuse = float3(1, 0, 1);
		m->Specular = float3(1, 1, 1);
		m->SpecularSharpness = 40;
		m->Roulette = float4(0.6, 0.8, 0, 0.66); // Mirror
	}
}

void InitializeProcess() {
	auto backBuffer = presenter->GetBackBuffer();
	switch (USED_PROCESS) {
	case BASIC:
		process = presenter->Load<BasicProcess>(ScreenDescription(backBuffer->getWidth(), backBuffer->getHeight()));
		break;
	case Debug_SGC:
		process = presenter->Load<DebugSGCProcess>(ScreenDescription(backBuffer->getWidth(), backBuffer->getHeight()));
		break;
	case Debug_AB:
		process = presenter->Load<DebugABProcess>(ScreenDescription(backBuffer->getWidth(), backBuffer->getHeight()));
		break;
	case Debug_EB:
		process = presenter->Load<DebugEBProcess>(ScreenDescription(backBuffer->getWidth(), backBuffer->getHeight()));
		break;
	case Debug_RT:
		process = presenter->Load<RTProcess>(ScreenDescription(backBuffer->getWidth(), backBuffer->getHeight()));
		break;
	}
	process->RenderTarget = backBuffer;
	process->SetScene(scene);
}

void InitializeGraphics(HWND hWnd) {

	presenter = new Presenter(hWnd, false);
	auto manager = presenter->GetManager();
	auto backBuffer = presenter->GetBackBuffer();

	InitializeScene(manager);

	InitializeProcess();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SAMPLEAPP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAMPLEAPP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_SAMPLEAPP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	InitializeGraphics(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool ldown = false;
	static bool rdown = false;
	static int lastXPos;
	static int lastYPos;
	static int xPos;
	static int yPos;

	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;

	case WM_LBUTTONDOWN:
		ldown = true;
		lastXPos = GET_X_LPARAM(lParam);
		lastYPos = GET_Y_LPARAM(lParam);
		break;
	case WM_RBUTTONDOWN:
		rdown = true;
		lastXPos = GET_X_LPARAM(lParam);
		lastYPos = GET_Y_LPARAM(lParam);
		break;

	case WM_LBUTTONUP:
		ldown = false;
		break;
	case WM_RBUTTONUP:
		rdown = false;
		break;

	case WM_MOUSEMOVE:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);

		if (ldown)
			scene->getCamera()->Rotate((xPos - lastXPos)*0.01, -(yPos - lastYPos)*0.001);
		if (rdown)
			scene->getCamera()->RotateAround((xPos - lastXPos)*0.01, -(yPos - lastYPos)*0.001);
		lastXPos = xPos;
		lastYPos = yPos;
		break;
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case 0x57: // W
			scene->getCamera()->MoveForward();
			break;
		case 0x41: // A
			scene->getCamera()->MoveLeft();
			break;
		case 0x44: // D
			scene->getCamera()->MoveRight();
			break;
		case 0x53: // S
			scene->getCamera()->MoveBackward();
			break;
		case 0x49: // I
			ShowInfo = !ShowInfo;
			break;
		case 0x46: // F
			DebugInfoChanging = DEBUG_INFO_FACE;
			break;
		case 0x4B: // K
			DebugInfoChanging = DEBUG_INFO_LEVEL;
			break;
		case 0x4C: // L
			DebugInfoChanging = DEBUG_INFO_LAYER;
			break;
		case 0x52: // R
			CurrentTime = GetTickCount64();
			frameCount = 1;
			nextShow = GetTimming();
			StartTimming();
			break;
		case 0x43: // C
		{
			RTProcess *rtP = dynamic_cast<RTProcess*>(process);
			if (rtP != nullptr)
			{
				rtP->ShowComplexity = !rtP->ShowComplexity;
			}
			break;
		}
		case 0x55: // U
		{
			RTProcess *rtP = dynamic_cast<RTProcess*>(process);
			if (rtP != nullptr)
			{
				rtP->UseAdaptiveSteps = !rtP->UseAdaptiveSteps;
			}
			break;
		}
		case VK_ADD: // +
		{
			auto dp = dynamic_cast<DebugableProcess*>(process);
			if (dp != nullptr)
				switch (DebugInfoChanging)
				{
				case DEBUG_INFO_FACE:
					dp->Debugging.FaceIndex = min(5, dp->Debugging.FaceIndex + 1);
					break;
				case DEBUG_INFO_LAYER:
					dp->Debugging.LayerIndex++;
					break;
				case DEBUG_INFO_LEVEL:
					dp->Debugging.Level++;
					break;
				}
			break;
		}
		case VK_SUBTRACT: // -
		{
			auto dp = dynamic_cast<DebugableProcess*>(process);
			if (dp != nullptr)
				switch (DebugInfoChanging)
				{
				case DEBUG_INFO_FACE:
					dp->Debugging.FaceIndex = max(0, dp->Debugging.FaceIndex - 1);
					break;
				case DEBUG_INFO_LAYER:
					dp->Debugging.LayerIndex = max(0, dp->Debugging.LayerIndex - 1);
					break;
				case DEBUG_INFO_LEVEL:
					dp->Debugging.Level = max(0, dp->Debugging.Level - 1);
					break;
				}
			break;
		}
		}
	}
	break;
	/*case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	break;*/
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void StartTimming()
{
	if (TimeIsRunning)
		return;

	CurrentTime = GetTickCount64() - CurrentTime;
	TimeIsRunning = true;
}

void StopTimming()
{
	if (!TimeIsRunning)
		return;

	CurrentTime = GetTickCount64() - CurrentTime;
	TimeIsRunning = false;
}

float GetTimming()
{
	if (TimeIsRunning)
		return (GetTickCount64() - CurrentTime) / 1000.0;
	return CurrentTime / 1000.0;
}

void ComputeFPS() {
	static bool started = false;
	if (!started)
	{
		StartTimming();
		started = true;
	}

	float t = GetTimming();
	
	frameCount++;

	if (GetTimming() > nextShow)
	{
		FPS = 1000 * t / frameCount;// t > 0 ? frameCount / t : 0;

		nextShow += 1;
	}
}

void Render()
{
	presenter->Run(process);

	StopTimming();

	if (ShowInfo)
	{
		string s = "";
		int value = 0;
		auto p = dynamic_cast<DebugableProcess*>(process);
		if (p != nullptr)
			switch (DebugInfoChanging) {
			case DEBUG_INFO_FACE:
				s = "Face";
				value = p->Debugging.FaceIndex;
				break;
			case DEBUG_INFO_LAYER:
				s = "Layer";
				value = p->Debugging.LayerIndex;
				break;
			case DEBUG_INFO_LEVEL:
				s = "Level";
				value = p->Debugging.Level;
				break;
			default:
				s = "None";
				value = 0;
				break;
			}

		ostringstream o(ios_base::out);
		o << "Changing " << s << " " << value;
		presenter->WriteLine(o.str().data());
		presenter->WriteLineW(std::to_wstring(FPS).data());
	}

	StartTimming();

	presenter->Present();

	ComputeFPS();
}