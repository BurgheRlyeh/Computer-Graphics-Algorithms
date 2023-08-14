#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <chrono>

#include "Texture.h"

#include "Cube.h"
#include "Sphere.h"
#include "Rect.h"

#define _MATH_DEFINES_DEFINED

class Cube;
class Sphere;
class Rect;

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

		float cameraRotationSpeed{ DirectX::XM_2PI };

		float forwardDelta{};
		float rightDelta{};

		void move(float delta);
		void getDirections(DirectX::XMFLOAT3& forward, DirectX::XMFLOAT3& right);
	} Camera;

	typedef struct MouseHandler {
		Renderer& renderer;

		MouseHandler() = delete;
		MouseHandler(Renderer& renderer):
			renderer(renderer)
		{}

		bool isMRBPressed{};
		int prevMouseX{};
		int prevMouseY{};

		void mouseRBPressed(bool isPressed, int x, int y);
		void mouseMoved(int x, int y);
		void mouseWheel(int delta);
	} MouseHandler;

	typedef struct KeyboardHandler {
		Renderer& renderer;

		KeyboardHandler() = delete;
		KeyboardHandler(Renderer& renderer):
			renderer(renderer)
		{}

	private:
		const float panSpeed{ 2.0 };

	public:
		void keyPressed(int keyCode);
		void keyReleased(int keyCode);
	} KeyboardHandler;

	ID3D11Device* m_pDevice{};
	ID3D11DeviceContext* m_pDeviceContext{};

	IDXGISwapChain* m_pSwapChain{};
	ID3D11RenderTargetView* m_pBackBufferRTV{};

	Cube* m_pCube{};
	float m_cubeAngleRotation{};

	Sphere* m_pSphere{};

	Rect* m_pRect{};

	ID3D11Buffer* m_pVPBuffer{};
	ID3D11RasterizerState* m_pRasterizerState{};
	ID3D11SamplerState* m_pSampler{};

	ID3D11Texture2D* m_pDepthBuffer;
	ID3D11DepthStencilView* m_pDepthBufferDSV;

	ID3D11DepthStencilState* m_pDepthState;
	ID3D11DepthStencilState* m_pTransDepthState;

	ID3D11BlendState* m_pTransBlendState;
	ID3D11BlendState* m_pOpaqueBlendState;

	UINT m_width{ 16 };
	UINT m_height{ 16 };

	Camera m_camera{};

	bool m_isModelRotate{};

	size_t m_prevTime{};

public:
	MouseHandler m_mouseHandler;
	KeyboardHandler m_keyboardHandler;

	Renderer():
		m_mouseHandler(*this),
		m_keyboardHandler(*this)
	{}

	bool init(HWND hWnd);
	void term();

	bool resize(UINT width, UINT height);
	bool update();
	bool render();

private:
	IDXGIAdapter* selectIDXGIAdapter(IDXGIFactory* factory);
	HRESULT createDeviceAndSwapChain(HWND hWnd, IDXGIAdapter* adapter);

	HRESULT setupBackBuffer();
	HRESULT createDepthBuffer();

	HRESULT initScene();
	void termScene();

	HRESULT createViewProjectionBuffer();
	HRESULT createRasterizerState();
	HRESULT createReversedDepthState();
	HRESULT createSampler();
};
