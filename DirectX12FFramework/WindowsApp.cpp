#include "WindowsApp.h"

bool WindowsApp::Init(int CmdShow) {

	if (mFullScreen) {
		HMONITOR hmon = MonitorFromWindow(mHwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hmon, &mi);

		mWidth = mi.rcMonitor.right - mi.rcMonitor.left;
		mHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
	}

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = mName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Error registering class",
			L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	mHwnd = CreateWindowEx(NULL,
		mName,
		mTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		mWidth, mHeight,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!mHwnd) {
		MessageBox(NULL, L"Error creating window",
			L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (mFullScreen) {
		SetWindowLong(mHwnd, GWL_STYLE, 0);
	}

	ShowWindow(mHwnd, CmdShow);
	UpdateWindow(mHwnd);

	if (!mDevice.InitD3D(mHwnd, mFullScreen)) {
		MessageBox(0, L"DirectX Initialization - Failed",
			L"Error", MB_OK);
		return false;
	}

	return true;
}
int WindowsApp::Run() {
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (mDevice.IsRunning()) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// run game code
			mDevice.Update();
			mDevice.Render();
		}
	}

	mDevice.WaitForPreviousFrame();
	CloseHandle(mDevice.GetFenceEvent());

	return 0;
}

LRESULT CALLBACK WindowsApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			if (MessageBox(0, L"Are you sure you want to exit?",
				L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES) {
				mDevice.SetRunning(false);
				DestroyWindow(hwnd);
			}
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

DirectXApp WindowsApp::mDevice;