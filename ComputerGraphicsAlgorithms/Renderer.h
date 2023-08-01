#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#define _MATH_DEFINES_DEFINED

class Renderer {
    typedef struct Vertex {
        DirectX::XMFLOAT3 point;
        COLORREF color;
    } Vertex;

    typedef struct ModelBuffer {
        DirectX::XMMATRIX m;
    } ModelBuffer;

    typedef struct ViewProjectionBuffer {
        DirectX::XMMATRIX vp;
    } ViewProjectionBuffer;

    typedef struct Camera {
        DirectX::XMFLOAT3 poi; // point of interest
        float r;    // distance to POI
        float angZ;    // angle in plane x0z (left-right)
        float angY; // angle from plane x0z (up-down)
    } Camera;

    const float cameraRotationSpeed{ DirectX::XM_PI * 2.0f };
    const float modelRotationSpeed{ DirectX::XM_PI / 2.0f };

    ID3D11Device* device{};
    ID3D11DeviceContext* deviceContext{};

    IDXGISwapChain* swapChain{};
    ID3D11RenderTargetView* backBufferRTV{};

    ID3D11Buffer* vertexBuffer{};
    ID3D11Buffer* indexBuffer{};

    ID3D11Buffer* modelBuffer{};
    ID3D11Buffer* viewProjectionBuffer{};

    ID3D11VertexShader* vertexShader{};
    ID3D11PixelShader* pixelShader{};
    ID3D11InputLayout* inputLayout{};

    ID3D11RasterizerState* rasterizerState{};

    UINT width{ 16 };
    UINT height{ 16 };

    Camera camera;
    bool isMRBPressed{};
    int prevMouseX{};
    int prevMouseY{};
    bool isModelRotate{};
    double angle{};

    size_t prevUSec{};

public:
    Renderer() = default;

    bool init(HWND hWnd);
    void term();

    bool update();
    bool render();
    bool resize(UINT width, UINT height);

    void mouseRBPressed(bool isPressed, int x, int y);
    void mouseMoved(int x, int y);
    void mouseWheel(int delta);
    void keyPressed(int keyCode);

private:
    IDXGIAdapter* selectIDXGIAdapter(IDXGIFactory* factory);
    HRESULT createDeviceAndSwapChain(HWND hWnd, IDXGIAdapter* adapter);

    HRESULT setupBackBuffer();

    HRESULT initScene();
    void termScene();

    HRESULT createVertexBuffer(Vertex (&vertices)[], UINT numVertices);
    HRESULT createIndexBuffer(USHORT (&indices)[], UINT numIndices);
    HRESULT createModelBuffer(ModelBuffer& modelBuffer);
    HRESULT createViewProjectionBuffer();
    HRESULT createRasterizerState();

    HRESULT compileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode = nullptr);
};
