#pragma once
#include "DirectXApp.h"

class WindowsApp {
public:
	WindowsApp(int width, int height, bool fullscreen, LPCTSTR name, LPCTSTR title) : mWidth(width), mHeight(height), 
																					  mFullScreen(fullscreen), mName(name), 
																					  mTitle(title){};
	~WindowsApp() {};

	bool Init(int CmdShow);
	int Run();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	static DirectXApp mDevice;
	HWND mHwnd;
	HINSTANCE hInstance;
	int mWidth, mHeight;
	bool mFullScreen;
	LPCTSTR mName, mTitle;
};