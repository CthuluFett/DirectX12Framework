#include "DirectXApp.h"
#include <d3d12.h>

#include "d3dx12.h"
#include <DXGI1_4.h>

DirectXApp::DirectXApp() {};
DirectXApp::~DirectXApp() {};

#define SAFE_RELEASE(x) if(x) { x->Release(); x = NULL; } 

bool DirectXApp::InitD3D(HWND hWnd, bool fullscreen) {

	//create the device
	IDXGIFactory4* dxgiFactory;
	HRESULT hR = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	if (FAILED(hR)) {
		return false;
	}

	IDXGIAdapter1* adapter; //graphics card
	int adapterIndex = 0;
	bool adapterFound = false;

	//find hardware that supports dx12
	while (dxgiFactory->EnumAdapters1(adapterIndex++, & adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		hR = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr);

		if (SUCCEEDED(hR)) {
			adapterFound = true;
			break;
		}
	}

	if (!adapterFound) {
		return false;
	}

	hR = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice));

	//create command queue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cqDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cqDesc.NodeMask = 0;

	hR = mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&mCommandQueue));

	if (FAILED(hR)) {
		return false;
	}

	//create swap chain
	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = 800;
	backBufferDesc.Height = 600;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferDesc.RefreshRate.Numerator = 1.0f;
	backBufferDesc.RefreshRate.Denominator = 60.0f;
	backBufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	backBufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = !fullscreen;
	swapChainDesc.Flags = 0;

	IDXGISwapChain* tempChain;
	hR = dxgiFactory->CreateSwapChain(mCommandQueue, &swapChainDesc, &tempChain);

	if (FAILED(hR)) {
		return false;
	}

	mSwapChain = static_cast<IDXGISwapChain3*>(tempChain);

	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

	//create back buffers
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = bufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hR = mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap));
	if (FAILED(hR)) {
		return false;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < bufferCount; i++) {
		hR = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i]));
		if (FAILED(hR)) {
			return false;
		}

		mDevice->CreateRenderTargetView(mRenderTargets[i], nullptr, rtvHandle);
		rtvHandle.Offset(1, mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

		hR = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator[i]));
		if (FAILED(hR)) {
			return false;
		}
	}

	hR = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator[0], nullptr, IID_PPV_ARGS(&mCommandList));
	if (FAILED(hR)) {
		return false;
	}

	mCommandList->Close();

	for (int i = 0; i < bufferCount; i++) {
		hR = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence[i]));
		if (FAILED(hR)) {
			return false;
		}
		mFenceValue[i] = 0;
	}

	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (mFenceEvent == nullptr) {
		return false;
	}

	return true;
}

void DirectXApp::Update() {

}

void DirectXApp::UpdatePipeline() {
	HRESULT hR;
	WaitForPreviousFrame();

	hR = mCommandAllocator[mFrameIndex]->Reset();
	if (FAILED(hR)) {
		mRunning = false;
	}

	hR = mCommandList->Reset(mCommandAllocator[mFrameIndex], nullptr);
	if (FAILED(hR)) {
		mRunning = false;
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hR = mCommandList->Close();
	if (FAILED(hR)) {
		mRunning = false;
	}
}

void DirectXApp::Render() {
	HRESULT hR;

	UpdatePipeline();

	ID3D12CommandList* pCommandLists[] = {mCommandList};
	mCommandQueue->ExecuteCommandLists(_countof(pCommandLists), pCommandLists);

	hR = mCommandQueue->Signal(mFence[mFrameIndex], mFenceValue[mFrameIndex]);
	if (FAILED(hR)) {
		mRunning = false;
	}

	hR = mSwapChain->Present(0, 0);
	if (FAILED(hR)) {
		mRunning = false;
	}
}

void DirectXApp::Cleanup() {
	
	// wait for the gpu to finish all frames
	for (int i = 0; i < bufferCount; ++i)
	{
		mFrameIndex = i;
		WaitForPreviousFrame();
	}

	// get swapchain out of full screen before exiting
	BOOL fs = false;
	if (mSwapChain->GetFullscreenState(&fs, NULL))
		mSwapChain->SetFullscreenState(false, NULL);

	SAFE_RELEASE(mDevice);
	SAFE_RELEASE(mSwapChain);
	SAFE_RELEASE(mCommandQueue);
	SAFE_RELEASE(mDescriptorHeap);
	SAFE_RELEASE(mCommandList);

	for (int i = 0; i < bufferCount; ++i)
	{
		SAFE_RELEASE(mRenderTargets[i]);
		SAFE_RELEASE(mCommandAllocator[i]);
		SAFE_RELEASE(mFence[i]);
	};
	
}

void DirectXApp::WaitForPreviousFrame() {
	HRESULT hr;

	// swap the current rtv buffer index so we draw on the correct buffer
	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

	// if the current fence value is still less than "fenceValue", then we know the GPU has not finished executing
	// the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
	if (mFence[mFrameIndex]->GetCompletedValue() < mFenceValue[mFrameIndex])
	{
		// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
		hr = mFence[mFrameIndex]->SetEventOnCompletion(mFenceValue[mFrameIndex], mFenceEvent);
		if (FAILED(hr))
		{
			mRunning = false;
		}

		// We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
		// has reached "fenceValue", we know the command queue has finished executing
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	// increment fenceValue for next frame
	mFenceValue[mFrameIndex]++;
}