#pragma once
#include <stdint.h>
#include <windows.h>
//DirectXApp.h - declaration of a class that encapsulates all necessary DirectX12 objects
//using pimpl idiom so forward declarations go here
struct ID3D12Device;
struct IDXGISwapChain3;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;

typedef void* HANDLE;

class DirectXApp {
public:
	DirectXApp();
	~DirectXApp();

	bool InitD3D(HWND hWnd, bool fullscreen);
	void Update();
	void UpdatePipeline();
	void Render();
	void Cleanup();
	void WaitForPreviousFrame();

	bool IsRunning() const { return mRunning;  }
	void SetRunning(bool isRunning) { mRunning = isRunning; }

	HANDLE GetFenceEvent()const { return mFenceEvent; }

	DirectXApp(const DirectXApp&) = delete;
	DirectXApp& operator=(const DirectXApp&) = delete;
private:
	const static int bufferCount = 3;

	ID3D12Device* mDevice;
	IDXGISwapChain3* mSwapChain;
	ID3D12CommandQueue* mCommandQueue;
	ID3D12DescriptorHeap* mDescriptorHeap;
	ID3D12Resource* mRenderTargets[bufferCount];
	ID3D12CommandAllocator* mCommandAllocator[bufferCount];
	ID3D12GraphicsCommandList* mCommandList;
	ID3D12Fence* mFence[bufferCount];
	HANDLE mFenceEvent;
	uint64_t mFenceValue[bufferCount];

	bool mRunning = true;

	int mFrameIndex;
	int rtvDescriptorSize;
};