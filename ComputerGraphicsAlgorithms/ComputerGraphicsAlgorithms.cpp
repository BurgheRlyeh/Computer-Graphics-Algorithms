// ComputerGraphicsAlgorithms.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ComputerGraphicsAlgorithms.h"
#include "Renderer.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

LONG                WindowWidth = 1280;
LONG                WindowHeight = 720;

Renderer* pRenderer = nullptr;

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow
) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_COMPUTERGRAPHICSALGORITHMS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

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
		OutputDebugString(_T("Render\n"));
		pRenderer->Render();
	}

	pRenderer->Term();
	delete pRenderer;

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

	pRenderer = new Renderer();
	if (!pRenderer->Init(hWnd)) {
		delete pRenderer;
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
	switch (message) {
		case WM_SIZE:
			if (pRenderer != nullptr) {
				RECT rc;
				GetClientRect(hWnd, &rc);
				pRenderer->Resize(rc.right - rc.left, rc.bottom - rc.top);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
