#pragma once

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d11.lib")

#include <dxgi.h>
#include <d3d11.h>

class Renderer {
public:
    Renderer():
        device{},
        deviceContext{},
        swapChain{},
        backBufferRTV{},
        width{ 16 },
        height{ 16 } {}

    bool Init(HWND hWnd);
    void Term();

    bool Render();
    bool Resize(UINT width, UINT height);

private:
    IDXGIAdapter* selectIDXGIAdapter(IDXGIFactory* factory);
    HRESULT createDeviceAndSwapChain(HWND hWnd, IDXGIAdapter* adapter);
    HRESULT SetupBackBuffer();

private:
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;

    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* backBufferRTV;

    UINT width;
    UINT height;
};
