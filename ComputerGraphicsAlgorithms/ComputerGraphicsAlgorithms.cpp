// ComputerGraphicsAlgorithms.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ComputerGraphicsAlgorithms.h"
#include "Renderer.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

LONG                WindowWidth{ 1920 };
LONG                WindowHeight{ 1080 };

Renderer* renderer{};

bool PressedKeys[0xff]{};

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow
) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	//srand(time(nullptr));
	srand(123456);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_COMPUTERGRAPHICSALGORITHMS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Fix working folder
	std::wstring dir;
	dir.resize(MAX_PATH + 1);
	GetCurrentDirectory(MAX_PATH + 1, &dir[0]);
	size_t configPos{ dir.find(L"x64") };
	if (configPos != std::wstring::npos) {
		dir.resize(configPos);
		dir += szTitle;
		SetCurrentDirectory(dir.c_str());
	}

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	HACCEL hAccelTable{ LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COMPUTERGRAPHICSALGORITHMS)) };

	MSG msg;

	// Main message loop:
	for (bool exit{}; !exit;) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (msg.message == WM_QUIT) {
				exit = true;
			}
		}

		if (renderer->update()) {
			renderer->render();
		}
	}

	renderer->term();
	delete renderer;

	return static_cast<int>(msg.wParam);
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex{
		.cbSize{ sizeof(WNDCLASSEX) },
		.style{ CS_HREDRAW | CS_VREDRAW },
		.lpfnWndProc{ WndProc },
		.cbClsExtra{},
		.cbWndExtra{},
		.hInstance{ hInstance },
		.hIcon{ LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COMPUTERGRAPHICSALGORITHMS)) },
		.hCursor{ LoadCursor(nullptr, IDC_ARROW) },
		.hbrBackground{ nullptr },
		.lpszMenuName{},
		.lpszClassName{ szWindowClass },
		.hIconSm{ LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL)) }
	};

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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd{ CreateWindowW(
			szWindowClass,
			szTitle,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			0,
			CW_USEDEFAULT,
			0,
			nullptr,
			nullptr,
			hInstance,
			nullptr
	) };

	if (!hWnd) {
		return FALSE;
	}

	renderer = new Renderer();
	if (!renderer->init(hWnd)) {
		delete renderer;
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Adjust window size
	RECT rc{
		.left{},
		.top{},
		.right{ WindowWidth },
		.bottom{ WindowHeight }
	};

	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

	MoveWindow(hWnd, 100, 100, rc.right - rc.left, rc.bottom - rc.top, TRUE);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
// 
//  WM_SIZE     - Resize window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
		return true;
	}

	if (message == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

	if (!renderer) {
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	switch (message) {
		case WM_SIZE: 
			RECT rc;
			GetClientRect(hWnd, &rc);
			renderer->resize(rc.right - rc.left, rc.bottom - rc.top);
			break;

		case WM_RBUTTONDOWN:
			renderer->m_mouseHandler.mouseRBPressed(true, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;

		case WM_RBUTTONUP:
			renderer->m_mouseHandler.mouseRBPressed(false, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;

		case WM_MOUSEMOVE:
			renderer->m_mouseHandler.mouseMoved(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;

		case WM_MOUSEWHEEL:
			renderer->m_mouseHandler.mouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			break;

		case WM_KEYDOWN:
			if (!PressedKeys[wParam]) {
				renderer->m_keyboardHandler.keyPressed((int)wParam);
				PressedKeys[wParam] = true;
			}
			break;

		case WM_KEYUP:
			if (PressedKeys[wParam]) {
				renderer->m_keyboardHandler.keyReleased((int)wParam);
			}
			PressedKeys[wParam] = false;
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
