//main.cpp
#include "DirectXApp.h"
#include <d3d12.h>
#include "WindowsApp.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int CmdShow) {
	
	WindowsApp winApp(800, 600, false, L"Framework", L"DirectX12");

	if (!winApp.Init(CmdShow)) return 1;

	return winApp.Run();
}