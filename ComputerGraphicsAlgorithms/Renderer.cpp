#include "framework.h"
#include "Renderer.h"
#include "Texture.h"

#include "../Common/imgui/imgui.h"
#include "../Common/imgui/backends/imgui_impl_dx11.h"
#include "../Common/imgui/backends/imgui_impl_win32.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void Renderer::MouseHandler::mouseRBPressed(bool isPressed, int x, int y) {
	isMRBPressed = isPressed;
	if (!isMRBPressed) {
		return;
	}

	prevMouseX = x;
	prevMouseY = y;
}

void Renderer::MouseHandler::mouseMoved(int x, int y) {
	if (!isMRBPressed) {
		return;
	}

	camera.angZ -= camera.rotationSpeed * (x - prevMouseX) / renderer.m_width;
	camera.angY += camera.rotationSpeed * (y - prevMouseY) / renderer.m_height;
	camera.angY = XMMax(camera.angY, 0.1f - XM_PIDIV2);
	camera.angY = XMMin(camera.angY, XM_PIDIV2 - 0.1f);

	prevMouseX = x;
	prevMouseY = y;
}

void Renderer::MouseHandler::mouseWheel(int delta) {
	camera.r = XMMax(camera.r - delta / 100.0f, 1.0f);
}

void Renderer::KeyboardHandler::keyPressed(int keyCode) {
	switch (keyCode) {
		case ' ':
			renderer.m_isModelRotate = !renderer.m_isModelRotate;
			break;

		case 'W':
		case 'w':
			camera.dForward -= panSpeed;
			break;

		case 'S':
		case 's':
			camera.dForward += panSpeed;
			break;

		case 'D':
		case 'd':
			camera.dRight -= panSpeed;
			break;

		case 'A':
		case 'a':
			camera.dRight += panSpeed;
			break;
	}
}

void Renderer::KeyboardHandler::keyReleased(int keyCode) {
	switch (keyCode) {
		case 'W':
		case 'w':
			renderer.m_camera.dForward += panSpeed;
			break;

		case 'S':
		case 's':
			renderer.m_camera.dForward -= panSpeed;
			break;

		case 'D':
		case 'd':
			renderer.m_camera.dRight += panSpeed;
			break;

		case 'A':
		case 'a':
			renderer.m_camera.dRight -= panSpeed;
			break;
	}
}

bool Renderer::init(HWND hWnd) {
	HRESULT hr{ S_OK };

	// Create a DirectX graphics interface factory.
	IDXGIFactory* factory{};
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	ThrowIfFailed(hr);

	IDXGIAdapter* adapter{ selectIDXGIAdapter(factory) };
	assert(adapter);

	hr = createDeviceAndSwapChain(hWnd, adapter);
	ThrowIfFailed(hr);

	hr = setupBackBuffer();
	ThrowIfFailed(hr);

	hr = initScene();
	ThrowIfFailed(hr);

	// initial camera setup
	m_camera = Camera{
		.r{ 5.f },
		.angZ{ -XM_PI / 2 },
		.angY{  XM_PI / 4 },
	};

	SAFE_RELEASE(adapter);
	SAFE_RELEASE(factory);

	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX11_Init(m_pDevice, m_pDeviceContext);

		m_sceneBuffer.lightCount.x = 1;
		m_sceneBuffer.lights[0].pos = { 1.1f, 1.0f, 0.0f, 1.0f };
		m_sceneBuffer.lights[0].color = { 1.0f, 1.0f, 0.0f, 0.0f };
		m_sceneBuffer.ambientColor = { 0.15f, 0.15f, 0.15f, 1.0f };
	}

	if (FAILED(hr)) {
		term();
	}

	return SUCCEEDED(hr);
}

void Renderer::term() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	termScene();

	SAFE_RELEASE(m_pBackBufferRTV);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pDeviceContext);

#ifdef _DEBUG
	if (m_pDevice) {
		ID3D11Debug* debug{};
		HRESULT hr{ m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug) };
		ThrowIfFailed(hr);

		if (debug->AddRef() != 3) {
			debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
		}

		debug->Release();

		SAFE_RELEASE(debug);
	}
#endif

	SAFE_RELEASE(m_pDevice);
}

IDXGIAdapter* Renderer::selectIDXGIAdapter(IDXGIFactory* factory) {
	assert(factory);

	IDXGIAdapter* adapter{};
	for (UINT idx{}; factory->EnumAdapters(idx, &adapter) != DXGI_ERROR_NOT_FOUND; ++idx) {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		if (wcscmp(desc.Description, L"Microsoft Basic Render Driver")) {
			return adapter;
		}

		adapter->Release();
	}

	return nullptr;
}

HRESULT Renderer::createDeviceAndSwapChain(HWND hWnd, IDXGIAdapter* adapter) {
	assert(hWnd);
	assert(adapter);

	D3D_FEATURE_LEVEL level, levels[]{ D3D_FEATURE_LEVEL_11_0 };

	UINT flags{};
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC swapChainDesc{
		.BufferDesc{
			.Width{ m_width },	// ширина 
			.Height{ m_height },	// высота
			.RefreshRate{	// частота обновления, 0 = inf
				.Numerator{},
				.Denominator{ 1 }
			},
			.Format{ DXGI_FORMAT_R8G8B8A8_UNORM },	// формат текстур
			.ScanlineOrdering{ DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED }, // порядок рисования по строк
			.Scaling{ DXGI_MODE_SCALING_UNSPECIFIED } // масштабирование
		},
		.SampleDesc{	// MSAA
			.Count{ 1 },
			.Quality{}
		},
		.BufferUsage{ DXGI_USAGE_RENDER_TARGET_OUTPUT },	// допустимое использование текстур (read/write)
		.BufferCount{ 2 },	// количество буфферов для рисования
		.OutputWindow{ hWnd },	// окно
		.Windowed{ true },	// исходный режим
		.SwapEffect{ DXGI_SWAP_EFFECT_DISCARD }, // алгоритм смены текстур (копирование оконным менеджером или нет)
		.Flags{}	// флажки
	};

	HRESULT hr{ D3D11CreateDeviceAndSwapChain(
		adapter,					// GPU
		D3D_DRIVER_TYPE_UNKNOWN,	// тип драйвера - Software реализация, например, для тестирования, не сломан ли драйвер GPU
		nullptr,					// дескриптор dll software реализации растеризатора
		flags,						// различные флаги
		levels,						// Уровени DirectX
		1,							// То же
		D3D11_SDK_VERSION,			// версия SDK
		&swapChainDesc,
		&m_pSwapChain,
		&m_pDevice,
		&level,
		&m_pDeviceContext
	) };

	assert(level == D3D_FEATURE_LEVEL_11_0);

	return hr;
}

bool Renderer::resize(UINT width, UINT height) {
	if (this->m_width == width && this->m_height == height) {
		return true;
	}

	SAFE_RELEASE(m_pBackBufferRTV);
	//SAFE_RELEASE(m_pDepthBuffer);
	//SAFE_RELEASE(m_pDepthBufferDSV);

	HRESULT hr{ m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0) };
	ThrowIfFailed(hr);

	this->m_width = width;
	this->m_height = height;

	hr = setupBackBuffer();
	ThrowIfFailed(hr);

	// setup skybox sphere
	float n{ 0.1f };
	float fov{ XM_PI / 3 };
	float halfW{ n * tanf(fov / 2) };
	float halfH{ static_cast<float>(height / width * halfW) };
	
	float r{ 1.1f * 2.0f * sqrtf(n * n + halfH * halfH + halfW * halfW) };

	m_pSphere->resize(r);

	return SUCCEEDED(hr);
}

bool Renderer::update() {
	size_t time{ static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now().time_since_epoch()
	).count()) };

	if (!m_prevTime) {
		m_prevTime = time;
	}

	// move camera
	m_camera.move((time - m_prevTime) / 1e6f);

	m_pCube->update((time - m_prevTime) / 1e6f, m_isModelRotate);

	// Move light bulb spheres
	m_pLightSphere->update(m_sceneBuffer.lights, m_sceneBuffer.lightCount.x);

	m_prevTime = time;

	Vector3 cameraPos{ m_camera.getPosition() };

	// Setup camera
	Matrix v{ XMMatrixLookAtLH(
		m_camera.getPosition(),
		m_camera.poi,
		m_camera.getUp()
	) };

	float nearPlane{ 0.1f };
	float farPlane{ 100.0f };
	float fov{ XM_PI / 3 };
	float aspectRatio{ static_cast<float>(m_height) / m_width };

	Matrix p{
		// Матрица построения перспективы для левой руки
		XMMatrixPerspectiveLH(
			2 * nearPlane * tanf(fov / 2), // width
			2 * nearPlane * tanf(fov / 2) * aspectRatio,	// height
			nearPlane, // rho to near
			farPlane // rho to far
		)
	};

	D3D11_MAPPED_SUBRESOURCE subresource;
	HRESULT hr = m_pDeviceContext->Map(
		m_pSceneBuffer, // указатель на ресурс
		0, // номер
		D3D11_MAP_WRITE_DISCARD, // не сохраняем предыдущее значение
		0, &subresource
	);
	ThrowIfFailed(hr);

	//ViewProjectionBuffer& sceneBuffer = *reinterpret_cast<ViewProjectionBuffer*>(subresource.pData);
	m_sceneBuffer.vp = v * p;
	m_sceneBuffer.cameraPos = { cameraPos.x, cameraPos.y, cameraPos.z, 0.0f };
	m_pCube->calcFrustum(
		m_camera,
		static_cast<float>(m_height) / m_width,
		m_sceneBuffer.frustum
	);
	
	memcpy(subresource.pData, &m_sceneBuffer, sizeof(SceneBuffer));

	m_pDeviceContext->Unmap(m_pSceneBuffer, 0);

	// update culling parameters
	m_pCube->updateCullParams();

	return SUCCEEDED(hr);
}

bool Renderer::render() {
	m_pDeviceContext->ClearState();

	ID3D11RenderTargetView* views[]{ m_pPostProcess->getBufferRTV()};
	// привязываем буфер глубины к Output-Merger этапу
	m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);
	//m_pDeviceContext->OMSetRenderTargets(1, views, m_pDepthBufferDSV);

	static const FLOAT BackColor[4]{ 0.25f, 0.25f, 0.25f, 1.0f };
	m_pDeviceContext->ClearRenderTargetView(m_pPostProcess->getBufferRTV(), BackColor);
	// очистка буфера глубины
	//m_pDeviceContext->ClearDepthStencilView(m_pDepthBufferDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);

	D3D11_VIEWPORT viewport{
		.TopLeftX{},
		.TopLeftY{},
		.Width{ static_cast<FLOAT>(m_width) },
		.Height{ static_cast<FLOAT>(m_height) },
		.MinDepth{},
		.MaxDepth{ 1.0f }
	};
	m_pDeviceContext->RSSetViewports(1, &viewport);

	D3D11_RECT rect{
		.left{},
		.top{},
		.right{ static_cast<LONG>(m_width) },
		.bottom{ static_cast<LONG>(m_width) }
	};
	m_pDeviceContext->RSSetScissorRects(1, &rect);

	m_pDeviceContext->OMSetDepthStencilState(m_pDepthState, 0);

	m_pDeviceContext->RSSetState(m_pRasterizerState);

	m_pDeviceContext->OMSetBlendState(m_pOpaqueBlendState, nullptr, 0xFFFFFFFF);

	m_pCube->cullBoxes(m_pSceneBuffer, m_camera, static_cast<float>(m_height) / m_width);

	m_pSphere->render(m_pSampler, m_pSceneBuffer);

	m_pCube->render(m_pSampler, m_pSceneBuffer);

	if (m_isShowLights) {
		m_pLightSphere->render(m_pSceneBuffer, m_pDepthState, m_pOpaqueBlendState, m_sceneBuffer.lightCount.x);
	}

	m_pRect->render(
		m_pSampler,
		m_pSceneBuffer,
		m_pTransDepthState,
		m_pTransBlendState,
		m_camera.getPosition()
	);

	m_pPostProcess->render(m_pBackBufferRTV, m_pSampler);

	m_pCube->readQueries();

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	{
		ImGui::Begin("Lights");

		ImGui::Checkbox("Show bulbs", &m_isShowLights);
		ImGui::Checkbox("Use normal maps", &m_isUseNormalMaps);
		ImGui::Checkbox("Show normals", &m_isShowNormals);
		ImGui::Checkbox("Use sepia", &m_pPostProcess->m_useSepia);

		m_sceneBuffer.lightCount.y = m_isUseNormalMaps ? 1 : 0;
		m_sceneBuffer.lightCount.z = m_isShowNormals ? 1 : 0;
		m_sceneBuffer.postProcess.x = m_pPostProcess->m_useSepia ? 1 : 0;

		bool add = ImGui::Button("+");
		ImGui::SameLine();
		bool remove = ImGui::Button("-");

		if (add && m_sceneBuffer.lightCount.x < 10) {
			m_sceneBuffer.lights[++m_sceneBuffer.lightCount.x - 1] = LightSphere::Light();
		}
		if (remove && m_sceneBuffer.lightCount.x > 0) {
			--m_sceneBuffer.lightCount.x;
		}

		char buffer[1024];
		for (int i = 0; i < m_sceneBuffer.lightCount.x; i++) {
			ImGui::Text("Light %d", i);
			sprintf_s(buffer, "Pos %d", i);
			ImGui::DragFloat3(buffer, (float*)&m_sceneBuffer.lights[i].pos, 0.1f, -10.0f, 10.0f);
			sprintf_s(buffer, "Color %d", i);
			ImGui::ColorEdit3(buffer, (float*)&m_sceneBuffer.lights[i].color);
		}

		ImGui::End();

		m_pCube->initImGUI();
		
		m_sceneBuffer.lightCount.w = static_cast<int>(m_pCube->getIsDoCull());
	}

	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return SUCCEEDED(m_pSwapChain->Present(0, 0));
}

HRESULT Renderer::setupBackBuffer() {
	ID3D11Texture2D* backBuffer{};
	HRESULT hr{ S_OK };

	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	ThrowIfFailed(hr);

	hr = m_pDevice->CreateRenderTargetView(backBuffer, nullptr, &m_pBackBufferRTV);
	SAFE_RELEASE(backBuffer);
	ThrowIfFailed(hr);

	//hr = createDepthBuffer();
	ThrowIfFailed(hr);

	//hr = SetResourceName(m_pDepthBuffer, "DepthBuffer");
	ThrowIfFailed(hr);

	//hr = m_pDevice->CreateDepthStencilView(m_pDepthBuffer, nullptr, &m_pDepthBufferDSV);
	ThrowIfFailed(hr);

	//hr = SetResourceName(m_pDepthBufferDSV, "DepthBufferView");
	ThrowIfFailed(hr);

	m_pPostProcess = new PostProcess(m_pDevice, m_pDeviceContext);
	hr = m_pPostProcess->init();
	ThrowIfFailed(hr);

	hr = m_pPostProcess->setupBuffer(m_width, m_height);
	ThrowIfFailed(hr);

	return hr;
}

HRESULT Renderer::createDepthBuffer() {
	D3D11_TEXTURE2D_DESC desc{
		.Width{ m_width },
		.Height{ m_height },
		.MipLevels{ 1 },
		.ArraySize{ 1 },
		// 32-битный формат с плавающей запятой для глубину.
		.Format{ DXGI_FORMAT_D32_FLOAT },
		.SampleDesc{ 1, 0 },
		.Usage{ D3D11_USAGE_DEFAULT },
		// использовать для depth-stencil
		.BindFlags{ D3D11_BIND_DEPTH_STENCIL },
	};

	//return m_pDevice->CreateTexture2D(&desc, nullptr, &m_pDepthBuffer);
	return S_OK;
}

HRESULT Renderer::initScene() {
	HRESULT hr{ S_OK };

	m_pCube = new Cube(m_pDevice, m_pDeviceContext);

	Matrix positions[]{
		XMMatrixIdentity(),
		XMMatrixTranslation(2.0f, 0.0f, 0.0f)
	};

	hr = m_pCube->init(positions, 2);
	ThrowIfFailed(hr);

	// create view-projection buffer
	{
		hr = createViewProjectionBuffer();
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pSceneBuffer, "SceneBuffer");
		ThrowIfFailed(hr);
	}

	// No culling rasterizer state
	{
		hr = createRasterizerState();
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pRasterizerState, "RasterizerState");
		ThrowIfFailed(hr);
	}

	// create blend states
	{
		D3D11_BLEND_DESC desc{
			// альфа-покрытие для MSAA
			.AlphaToCoverageEnable{},
			// независимые настройки смешивания (до 8)
			.IndependentBlendEnable{},
			.RenderTarget{ {
				.BlendEnable{ TRUE },
					// коэффициенты смешивания (текущий и из PS)
					.SrcBlend{ D3D11_BLEND_SRC_ALPHA }, // alpha
					.DestBlend{ D3D11_BLEND_INV_SRC_ALPHA }, // 1 - alpha
					// как комбинировать SrcBlend и DestBlend
					.BlendOp{ D3D11_BLEND_OP_ADD },
					.SrcBlendAlpha{ D3D11_BLEND_ONE },
					.DestBlendAlpha{ D3D11_BLEND_ZERO },
					.BlendOpAlpha{ D3D11_BLEND_OP_ADD },
					// маска для записи (хранить во всех значениях)
					.RenderTargetWriteMask{ D3D11_COLOR_WRITE_ENABLE_ALL }
				} }
		};

		hr = m_pDevice->CreateBlendState(&desc, &m_pTransBlendState);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pTransBlendState, "TransBlendState");
		ThrowIfFailed(hr);

		desc.RenderTarget[0].BlendEnable = FALSE;
		hr = m_pDevice->CreateBlendState(&desc, &m_pOpaqueBlendState);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pOpaqueBlendState, "OpaqueBlendState");
		ThrowIfFailed(hr);
	}

	// create reverse depth state
	{
		hr = createReversedDepthState();
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pDepthState, "DephtState");
		ThrowIfFailed(hr);
	}

	// create reverse transparent depth state
	{
		D3D11_DEPTH_STENCIL_DESC desc{
			.DepthEnable{ TRUE },
			.DepthWriteMask{ D3D11_DEPTH_WRITE_MASK_ZERO },
			.DepthFunc{ D3D11_COMPARISON_GREATER },
			.StencilEnable{ FALSE }
		};

		hr = m_pDevice->CreateDepthStencilState(&desc, &m_pTransDepthState);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pTransDepthState, "TransDepthState");
		ThrowIfFailed(hr);
	}

	// create sampler
	{
		hr = createSampler();
		ThrowIfFailed(hr);
	}

	m_pSphere = new Sphere(m_pDevice, m_pDeviceContext);
	hr = m_pSphere->init();
	ThrowIfFailed(hr);

	m_pRect = new Rect(m_pDevice, m_pDeviceContext);
	Vector3 rectPositions[]{
		{ 1.0f, 0, 0.0f },
		{ 1.2f, 0, 0.0f }
	};
	Vector4 rectColors[]{
		{ 1.0f, 0.25f, 0.0f, 0.5f },
		{ 0.0f, 0.25f, 1.0f, 0.5f }
	};
	hr = m_pRect->init(rectPositions, rectColors, 2);
	ThrowIfFailed(hr);

	m_pLightSphere = new LightSphere(m_pDevice, m_pDeviceContext);
	hr = m_pLightSphere->init();
	ThrowIfFailed(hr);

	hr = m_pCube->initCull();
	ThrowIfFailed(hr);

	return hr;
}

void Renderer::termScene() {
	SAFE_RELEASE(m_pSceneBuffer);
	SAFE_RELEASE(m_pRasterizerState);
	SAFE_RELEASE(m_pSampler);

	SAFE_RELEASE(m_pDepthState);
	SAFE_RELEASE(m_pTransDepthState);

	SAFE_RELEASE(m_pTransBlendState);
	SAFE_RELEASE(m_pOpaqueBlendState);

	//SAFE_RELEASE(m_pDepthBuffer);
	//SAFE_RELEASE(m_pDepthBufferDSV);

	m_pCube->term();
	m_pSphere->term();
}

HRESULT Renderer::createViewProjectionBuffer() {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(SceneBuffer) },
		.Usage{ D3D11_USAGE_DYNAMIC },	// часто собираемся обновлять, храним ближе к CPU
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER },
		.CPUAccessFlags{ D3D11_CPU_ACCESS_WRITE }	// можем писать со стороны CPU  
	};

	return m_pDevice->CreateBuffer(&desc, nullptr, &m_pSceneBuffer);
}

HRESULT Renderer::createRasterizerState() {
	D3D11_RASTERIZER_DESC desc{
		.FillMode{ D3D11_FILL_SOLID },
		.CullMode{ D3D11_CULL_BACK },
		.DepthClipEnable{ TRUE }
	};

	return m_pDevice->CreateRasterizerState(&desc, &m_pRasterizerState);
}

HRESULT Renderer::createReversedDepthState() {
	D3D11_DEPTH_STENCIL_DESC desc{
		.DepthEnable{ TRUE },
		// запись в буфер глубины
		.DepthWriteMask{ D3D11_DEPTH_WRITE_MASK_ALL },
		// функция сравнения глубин
		.DepthFunc{ D3D11_COMPARISON_GREATER_EQUAL }
	};

	return m_pDevice->CreateDepthStencilState(&desc, &m_pDepthState);
}

HRESULT Renderer::createSampler() {
	D3D11_SAMPLER_DESC desc{
		// метод фильтрации
		.Filter{ D3D11_FILTER_ANISOTROPIC },
		// методы обработки координат за границами
		// WRAP - повторение текстуры
		.AddressU{ D3D11_TEXTURE_ADDRESS_WRAP },
		.AddressV{ D3D11_TEXTURE_ADDRESS_WRAP },
		.AddressW{ D3D11_TEXTURE_ADDRESS_WRAP },
		.MipLODBias{}, // смещение уровня mipmap
		.MaxAnisotropy{ 16 }, // настройка аниз фильтр
		.ComparisonFunc{ D3D11_COMPARISON_NEVER },
		// цвет для ADDRESS_BORDER
		.BorderColor{ 1.0f, 1.0f, 1.0f, 1.0f },
		// диапазон mipmap
		.MinLOD{ -FLT_MAX },
		.MaxLOD{ FLT_MAX }
	};

	return m_pDevice->CreateSamplerState(&desc, &m_pSampler);
}
