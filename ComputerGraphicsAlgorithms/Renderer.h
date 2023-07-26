#pragma once

#include <dxgi.h>
#include <d3d11.h>

class Renderer {
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;

    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* backBufferRTV;

    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;

    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;
    ID3D11InputLayout* inputLayout;

    UINT width;
    UINT height;

    typedef struct Vertex {
        float x, y, z;
        COLORREF color;
    } Vertex;

public:
    Renderer():
        device{},
        deviceContext{},
        swapChain{},
        backBufferRTV{},
        width{ 16 },
        height{ 16 },
        vertexBuffer{},
        indexBuffer{},
        vertexShader{},
        pixelShader{},
        inputLayout{} {};

    bool init(HWND hWnd);
    void term();

    bool render();
    bool resize(UINT width, UINT height);

private:
    IDXGIAdapter* selectIDXGIAdapter(IDXGIFactory* factory);
    HRESULT createDeviceAndSwapChain(HWND hWnd, IDXGIAdapter* adapter);

    HRESULT setupBackBuffer();

    HRESULT initScene();
    HRESULT createVertexBuffer(Vertex (&vertices)[], UINT numVertices);
    HRESULT createIndexBuffer(USHORT (&indices)[], UINT numIndices);

    HRESULT compileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode = nullptr);
};
