#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <chrono>

#include "Texture.h"

#include "Cube.h"
#include "Sphere.h"

#define _MATH_DEFINES_DEFINED

class Cube;
class Sphere;

class Renderer {
    typedef struct ViewProjectionBuffer {
        DirectX::XMMATRIX vp;
        DirectX::XMFLOAT4 cameraPos;
    } ViewProjectionBuffer;

    typedef struct Camera {
        DirectX::XMFLOAT3 poi; // point of interest
        float r;    // distance to POI
        float angZ;    // angle in plane x0z (left-right)
        float angY; // angle from plane x0z (up-down)

        void getDirections(DirectX::XMFLOAT3& forward, DirectX::XMFLOAT3& right);
    } Camera;

    const float cameraRotationSpeed{ DirectX::XM_2PI };
    const float modelRotationSpeed{ DirectX::XM_PIDIV2 };

    ID3D11Device* device{};
    ID3D11DeviceContext* deviceContext{};

    IDXGISwapChain* swapChain{};
    ID3D11RenderTargetView* backBufferRTV{};

    Cube* m_pCube{};
    Sphere* m_pSphere{};

    ID3D11Buffer* viewProjectionBuffer{};
    ID3D11RasterizerState* rasterizerState{};
    ID3D11SamplerState* sampler{};

    UINT width{ 16 };
    UINT height{ 16 };

    Camera camera{};

    bool isMRBPressed{};
    int prevMouseX{};
    int prevMouseY{};

    bool isModelRotate{};
    double angle{};

    const float panSpeed{ 2.0 };

    float forwardDelta{};
    float rightDelta{};

    size_t prevUSec{};

public:
    bool init(HWND hWnd);
    void term();

    bool resize(UINT width, UINT height);
    bool update();
    bool render();

    void mouseRBPressed(bool isPressed, int x, int y);
    void mouseMoved(int x, int y);
    void mouseWheel(int delta);

    void keyPressed(int keyCode);
    void keyReleased(int keyCode);

private:
    IDXGIAdapter* selectIDXGIAdapter(IDXGIFactory* factory);
    HRESULT createDeviceAndSwapChain(HWND hWnd, IDXGIAdapter* adapter);

    HRESULT setupBackBuffer();

    HRESULT initScene();
    void termScene();

    HRESULT createViewProjectionBuffer();
    HRESULT createRasterizerState();
    HRESULT createSampler();
};
