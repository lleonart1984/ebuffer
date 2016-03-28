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
#define USED_PROCESS Debug_AB

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
void Render();

// Rendering System
Presenter *presenter;
SScene *scene;
DrawSceneProcess *process;

// GUI parameters
bool ShowInfo = false;
ULONGLONG CurrentTime = 0;
bool TimeIsRunning = false;
float FPS;

void InitializeScene(DeviceManager* manager)
{
	scene = new Scene<VERTEX, MATERIAL>(manager);
	// Camera setup
	scene->getCamera()->Position = float3(1, 2, -6);
	scene->getCamera()->Target = float3(0, 2, 0);
	scene->getCamera()->Up = float3(0, 1, 0);
	scene->getCamera()->FoV = PI / 2;
	scene->getCamera()->NearPlane = 0.1;
	scene->getCamera()->FarPlane = 1000;

	// Light setup
	scene->getLight()->Position = float3(1, 14, -6);
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
		default:
			break;
		case 0x49: // I
			ShowInfo = !ShowInfo;
			break;
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

	static float nextShow = GetTimming() + 0;

	float t = GetTimming();

	static ULONGLONG frameCount = 0;

	frameCount++;

	if (GetTimming() > nextShow)
	{
		FPS = 1000*t / frameCount;// t > 0 ? frameCount / t : 0;

		nextShow += 1;
	}
}

void Render()
{
	presenter->Run(process);

	if (ShowInfo)
		presenter->DrawTextW(std::to_wstring(FPS).data());
	
	presenter->Present();

	ComputeFPS();
}