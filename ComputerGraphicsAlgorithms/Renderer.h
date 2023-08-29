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
#include "LightSphere.h"
#include "Camera.h"
#include "AABB.h"
#include "PostProcess.h"

#include "SimpleMath.h"

#define _MATH_DEFINES_DEFINED

class Cube;
class Sphere;
class Rect;
class LightSphere;
struct Camera;
struct AABB;
struct PostProcess;

class Renderer {
	typedef struct SceneBuffer {
		DirectX::SimpleMath::Matrix vp{};
		DirectX::SimpleMath::Vector4 cameraPos{};
		DirectX::XMINT4 lightCount{};
		DirectX::XMINT4 postProcess{}; // x - use sepia
		LightSphere::Light lights[10]{};
		DirectX::SimpleMath::Color ambientColor{};
		DirectX::SimpleMath::Plane frustum[6]{};
	} SceneBuffer;

	typedef struct MouseHandler {
		Renderer& renderer;
		Camera& camera;

		MouseHandler() = delete;
		MouseHandler(Renderer& renderer, Camera& camera):
			renderer(renderer),
			camera(camera)
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
		Camera& camera;

		KeyboardHandler() = delete;
		KeyboardHandler(Renderer& renderer, Camera& camera):
			renderer(renderer),
			camera(camera)
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

	Camera m_camera{};

	PostProcess* m_pPostProcess{};

	Cube* m_pCube{};
	float m_cubeAngleRotation{};

	Sphere* m_pSphere{};

	Rect* m_pRect{};

	LightSphere* m_pLightSphere{};

	ID3D11Buffer* m_pSceneBuffer{};
	ID3D11RasterizerState* m_pRasterizerState{};
	ID3D11SamplerState* m_pSampler{};

	ID3D11Texture2D* m_pDepthBuffer{};
	ID3D11DepthStencilView* m_pDepthBufferDSV{};

	ID3D11DepthStencilState* m_pDepthState{};
	ID3D11DepthStencilState* m_pTransDepthState{};

	ID3D11BlendState* m_pTransBlendState{};
	ID3D11BlendState* m_pOpaqueBlendState{};

	UINT m_width{ 16 };
	UINT m_height{ 16 };

	bool m_isModelRotate{ true };

	size_t m_prevTime{};

	SceneBuffer m_sceneBuffer{};

	bool m_isShowLights{ true };
	bool m_isUseNormalMaps{ true };
	bool m_isShowNormals{};
	bool m_isUseAmbient{ true };
	bool m_useSepia{};

public:
	MouseHandler m_mouseHandler;
	KeyboardHandler m_keyboardHandler;

	Renderer():
		m_mouseHandler(*this, m_camera),
		m_keyboardHandler(*this, m_camera)
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
