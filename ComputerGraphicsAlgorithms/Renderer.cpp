#include "framework.h"
#include "Renderer.h"
#include "Texture.h"

void Renderer::Camera::move(float delta) {
	DirectX::XMFLOAT3 cf, cr;
	getDirections(cf, cr);
	poi = DirectX::XMFLOAT3(
		poi.x + (cf.x * forwardDelta + cr.x * rightDelta) * delta,
		poi.y + (cf.y * forwardDelta + cr.y * rightDelta) * delta,
		poi.z + (cf.z * forwardDelta + cr.z * rightDelta) * delta
	);
}

void Renderer::Camera::getDirections(DirectX::XMFLOAT3& forward, DirectX::XMFLOAT3& right) {
	DirectX::XMFLOAT3 dir{
		cosf(angY) * cosf(angZ),
		sinf(angY),
		cosf(angY) * sinf(angZ)
	};
	DirectX::XMFLOAT3 up{
		cosf(angY + DirectX::XM_PIDIV2)* cosf(angZ),
		sinf(angY + DirectX::XM_PIDIV2),
		cosf(angY + DirectX::XM_PIDIV2)* sinf(angZ)
	};
	right = DirectX::XMFLOAT3(
		 up.y * dir.z - up.z * dir.y,
		-up.x * dir.z + up.z * dir.x,
		 up.x * dir.y - up.y * dir.x
	);
	right.y = 0.0f;

	right.x /= sqrt(pow(right.x, 2) + pow(right.y, 2) + pow(right.z, 2));
	right.z /= sqrt(pow(right.x, 2) + pow(right.y, 2) + pow(right.z, 2));

	if (fabs(dir.x) > 1e-5f || fabs(dir.z) > 1e-5f) {
		forward = DirectX::XMFLOAT3(dir.x, 0.0f, dir.z);
	} else {
		forward = DirectX::XMFLOAT3(up.x, 0.0f, up.z);
	}

	forward.x /= sqrt(pow(forward.x, 2) + pow(forward.y, 2) + pow(forward.z, 2));
	forward.z /= sqrt(pow(forward.x, 2) + pow(forward.y, 2) + pow(forward.z, 2));
}

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

	renderer.camera.angZ += -(float)(x - prevMouseX) / renderer.width * renderer.camera.cameraRotationSpeed;
	renderer.camera.angY += (float)(y - prevMouseY) / renderer.width * renderer.camera.cameraRotationSpeed;
	renderer.camera.angY = DirectX::XMMax(renderer.camera.angY, -renderer.m_pCube->getModelRotationSpeed());
	renderer.camera.angY = DirectX::XMMin(renderer.camera.angY, renderer.m_pCube->getModelRotationSpeed());

	prevMouseX = x;
	prevMouseY = y;
}

void Renderer::MouseHandler::mouseWheel(int delta) {
	renderer.camera.r = DirectX::XMMax(renderer.camera.r - delta / 100.0f, 1.0f);
}

void Renderer::KeyboardHandler::keyPressed(int keyCode) {
	switch (keyCode) {
		case ' ':
			renderer.isModelRotate = !renderer.isModelRotate;
			break;

		case 'W':
		case 'w':
			renderer.camera.forwardDelta -= panSpeed;
			break;

		case 'S':
		case 's':
			renderer.camera.forwardDelta += panSpeed;
			break;

		case 'D':
		case 'd':
			renderer.camera.rightDelta -= panSpeed;
			break;

		case 'A':
		case 'a':
			renderer.camera.rightDelta += panSpeed;
			break;
	}
}

void Renderer::KeyboardHandler::keyReleased(int keyCode) {
	switch (keyCode) {
		case 'W':
		case 'w':
			renderer.camera.forwardDelta += panSpeed;
			break;

		case 'S':
		case 's':
			renderer.camera.forwardDelta -= panSpeed;
			break;

		case 'D':
		case 'd':
			renderer.camera.rightDelta += panSpeed;
			break;

		case 'A':
		case 'a':
			renderer.camera.rightDelta -= panSpeed;
			break;
	}
}

bool Renderer::init(HWND hWnd) {
	HRESULT hr{ S_OK };

	// Create a DirectX graphics interface factory.
	IDXGIFactory* factory{};
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(hr)) {
		return false;
	}

	IDXGIAdapter* adapter{ selectIDXGIAdapter(factory) };
	assert(adapter);

	hr = createDeviceAndSwapChain(hWnd, adapter);
	if (FAILED(hr)) {
		return false;
	}

	hr = setupBackBuffer();
	if (FAILED(hr)) {
		return false;
	}

	hr = initScene();
	if (FAILED(hr)) {
		return false;
	}

	// initial camera setup
	camera = Camera{
		.poi{ 0.0f, 0.0f, 0.0f },
		.r{ 5.0f },
		.angZ{ - DirectX::XM_PI / 2 },
		.angY{ DirectX::XM_PI / 4 },
	};

	SAFE_RELEASE(adapter);
	SAFE_RELEASE(factory);

	if (FAILED(hr)) {
		term();
	}

	return SUCCEEDED(hr);
}

void Renderer::term() {
	SAFE_RELEASE(backBufferRTV);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(deviceContext);

#ifdef _DEBUG
	if (device) {
		ID3D11Debug* debug{};
		HRESULT hr{ device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug) };
		if (FAILED(hr)) {
			return;
		}
		if (debug->AddRef() != 3) {
			debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
		}

		debug->Release();

		SAFE_RELEASE(debug);
	}
#endif

	SAFE_RELEASE(device);
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
			.Width{ width },	// ширина 
			.Height{ height },	// высота
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
		&swapChain,
		&device,
		&level,
		&deviceContext
	) };

	assert(level == D3D_FEATURE_LEVEL_11_0);

	return hr;
}

bool Renderer::resize(UINT width, UINT height) {
	if (this->width == width && this->height == height) {
		return true;
	}

	SAFE_RELEASE(backBufferRTV);
	SAFE_RELEASE(m_pDepthBuffer);
	SAFE_RELEASE(m_pDepthBufferDSV);

	HRESULT hr{ swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0) };
	if (FAILED(hr)) {
		return false;
	}

	this->width = width;
	this->height = height;

	hr = setupBackBuffer();
	if (FAILED(hr)) {
		return false;
	}

	// setup skybox sphere
	float n{ 0.1f };
	float fov{ DirectX::XM_PI / 3 };
	float halfW{ n * tanf(fov / 2) };
	float halfH{ static_cast<float>(height / width * halfW) };

	float r{ 1.1f * 2.0f * sqrtf(n * n + halfH * halfH + halfW * halfW) };

	m_pSphere->resize(r);

	return SUCCEEDED(hr);
}

bool Renderer::update() {
	size_t usec{ static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now().time_since_epoch()
	).count()) };

	if (!prevUSec) {
		prevUSec = usec;
	}

	// move camera
	camera.move((usec - prevUSec) / 1e6f);

	if (isModelRotate) {

		m_cubeAngleRotation += m_pCube->getModelRotationSpeed() * (usec - prevUSec) / 1e6f;

		m_pCube->update(
			0,
			// матрица вращения вокруг оси
			DirectX::XMMatrixRotationAxis(
				// вектор, описывающий ось вращения
				DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
				m_cubeAngleRotation
			)
		);

		m_pCube->update(
			1,
			DirectX::XMMatrixTranslation(2.0f, 0.0f, 0.0f)
		);
	}

	prevUSec = usec;

	DirectX::XMFLOAT3 cameraPos{
		camera.poi.x + camera.r * cosf(camera.angY) * cosf(camera.angZ),
		camera.poi.y + camera.r * sinf(camera.angY),
		camera.poi.z + camera.r * cosf(camera.angY) * sinf(camera.angZ)
	};

	// Setup camera
	DirectX::XMMATRIX v{
		// создание матрицы для системы координат левой руки
		DirectX::XMMatrixLookAtLH(
			// позиция камеры
			DirectX::XMVectorSet(
				cameraPos.x,
				cameraPos.y,
				cameraPos.z,
				0.0f
			),
			// позиция точки фокуса
			DirectX::XMVectorSet(
				camera.poi.x,
				camera.poi.y,
				camera.poi.z,
				0.0f
			),
			// направление вверх от камеры
			DirectX::XMVectorSet(
				cosf(camera.angY + DirectX::XM_PIDIV2) * cosf(camera.angZ),
				sinf(camera.angY + DirectX::XM_PIDIV2),
				cosf(camera.angY + DirectX::XM_PIDIV2) * sinf(camera.angZ),
				0.0f
			)
		)
	};

	float nearPlane{ 0.1f };
	float farPlane{ 100.0f };
	float fov{ DirectX::XM_PI / 3 };
	float aspectRatio{ static_cast<float>(height) / width };

	DirectX::XMMATRIX p{
		// Матрица построения перспективы для левой руки
		DirectX::XMMatrixPerspectiveLH(
			2 * farPlane * tanf(fov / 2),					// Ширина усеченного конуса в nearPlane
			2 * farPlane * tanf(fov / 2) * aspectRatio,	// Высота усеченного конуса в nearPlane
			farPlane,										// Расстояние до ближайшей плоскости отсечения
			nearPlane										// Расстояние до дальней плоскости отсечения
		)
	};

	D3D11_MAPPED_SUBRESOURCE subresource;
	HRESULT hr = deviceContext->Map(
		viewProjectionBuffer, // указатель на ресурс
		0, // номер
		D3D11_MAP_WRITE_DISCARD, // не сохраняем предыдущее значение
		0, &subresource
	);
	if (FAILED(hr)) {
		return false;
	}

	ViewProjectionBuffer& sceneBuffer = *reinterpret_cast<ViewProjectionBuffer*>(subresource.pData);
	sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);
	sceneBuffer.cameraPos = DirectX::XMFLOAT4(cameraPos.x, cameraPos.y, cameraPos.z, 0.0f);
	deviceContext->Unmap(this->viewProjectionBuffer, 0);

	return SUCCEEDED(hr);
}

bool Renderer::render() {
	deviceContext->ClearState();

	ID3D11RenderTargetView* views[]{ backBufferRTV };
	deviceContext->OMSetRenderTargets(1, views, m_pDepthBufferDSV);

	static const FLOAT BackColor[4]{ 0.25f, 0.25f, 0.25f, 1.0f };
	deviceContext->ClearRenderTargetView(backBufferRTV, BackColor);
	deviceContext->ClearDepthStencilView(m_pDepthBufferDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);

	D3D11_VIEWPORT viewport{
		.TopLeftX{},
		.TopLeftY{},
		.Width{ static_cast<FLOAT>(width) },
		.Height{ static_cast<FLOAT>(height) },
		.MinDepth{},
		.MaxDepth{ 1.0f }
	};
	deviceContext->RSSetViewports(1, &viewport);

	D3D11_RECT rect{
		.left{},
		.top{},
		.right{ static_cast<LONG>(width) },
		.bottom{ static_cast<LONG>(width) }
	};
	deviceContext->RSSetScissorRects(1, &rect);

	deviceContext->OMSetDepthStencilState(m_pDepthState, 0);

	deviceContext->RSSetState(rasterizerState);

	deviceContext->OMSetBlendState(m_pOpaqueBlendState, nullptr, 0xFFFFFFFF);

	m_pCube->render(sampler, viewProjectionBuffer);

	m_pSphere->render(sampler, viewProjectionBuffer);

	DirectX::XMFLOAT3 cameraPos{
		camera.poi.x + camera.r * cosf(camera.angY) * cosf(camera.angZ),
		camera.poi.y + camera.r * sinf(camera.angY),
		camera.poi.z + camera.r * cosf(camera.angY) * sinf(camera.angZ)
	};

	m_pRect->render(
		sampler,
		viewProjectionBuffer,
		m_pTransDepthState,
		m_pTransBlendState,
		cameraPos
	);

	return SUCCEEDED(swapChain->Present(0, 0));
}

HRESULT Renderer::setupBackBuffer() {
	ID3D11Texture2D* backBuffer{};
	HRESULT hr{ swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer) };
	if (FAILED(hr)) {
		return hr;
	}

	hr = device->CreateRenderTargetView(backBuffer, nullptr, &backBufferRTV);
	SAFE_RELEASE(backBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	D3D11_TEXTURE2D_DESC desc{
		.Width{ width },
		.Height{ height },
		.MipLevels{ 1 },
		.ArraySize{ 1 },
		.Format{ DXGI_FORMAT_D32_FLOAT },
		.SampleDesc{ 1, 0 },
		.Usage{ D3D11_USAGE_DEFAULT },
		.BindFlags{ D3D11_BIND_DEPTH_STENCIL },
		.CPUAccessFlags{},
		.MiscFlags{}
	};

	hr = device->CreateTexture2D(&desc, nullptr, &m_pDepthBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	hr = SetResourceName(m_pDepthBuffer, "DepthBuffer");
	if (FAILED(hr)) {
		return hr;
	}

	device->CreateDepthStencilView(m_pDepthBuffer, nullptr, &m_pDepthBufferDSV);
	if (FAILED(hr)) {
		return hr;
	}

	hr = SetResourceName(m_pDepthBufferDSV, "DepthBufferView");
	if (FAILED(hr)) {
		return hr;
	}


	return hr;
}

HRESULT Renderer::initScene() {
	HRESULT hr{ S_OK };

	m_pCube = new Cube(device, deviceContext);
	hr = m_pCube->init(2);
	if (FAILED(hr)) {
		return hr;
	}

	// create view-projection buffer
	{
		hr = createViewProjectionBuffer();
		if (FAILED(hr)) { 
			return hr;
		}

		hr = SetResourceName(viewProjectionBuffer, "SceneBuffer");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// No culling rasterizer state
	{
		hr = createRasterizerState();
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(rasterizerState, "RasterizerState");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create blend states
	{
		D3D11_BLEND_DESC desc{
			.AlphaToCoverageEnable{},
			.IndependentBlendEnable{},
			.RenderTarget{ {
				.BlendEnable{ TRUE },
				.SrcBlend{ D3D11_BLEND_SRC_ALPHA },
				.DestBlend{ D3D11_BLEND_INV_SRC_ALPHA },
				.BlendOp{ D3D11_BLEND_OP_ADD },
				.SrcBlendAlpha{ D3D11_BLEND_ONE },
				.DestBlendAlpha{ D3D11_BLEND_ZERO },
				.BlendOpAlpha{ D3D11_BLEND_OP_ADD },
				.RenderTargetWriteMask{ 
					D3D11_COLOR_WRITE_ENABLE_RED
						| D3D11_COLOR_WRITE_ENABLE_GREEN
						| D3D11_COLOR_WRITE_ENABLE_BLUE
				}
			} }
		};

		hr = device->CreateBlendState(&desc, &m_pTransBlendState);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pTransBlendState, "TransBlendState");
		if (FAILED(hr)) {
			return hr;
		}

		desc.RenderTarget[0].BlendEnable = FALSE;
		hr = device->CreateBlendState(&desc, &m_pOpaqueBlendState);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pOpaqueBlendState, "OpaqueBlendState");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create reverse depth state
	{
		D3D11_DEPTH_STENCIL_DESC desc{
			.DepthEnable{ TRUE },
			.DepthWriteMask{ D3D11_DEPTH_WRITE_MASK_ALL },
			.DepthFunc{ D3D11_COMPARISON_GREATER_EQUAL },
			.StencilEnable{}
		};

		hr = device->CreateDepthStencilState(&desc, &m_pDepthState);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pDepthState, "DephtState");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create reverse transparent depth state
	{
		D3D11_DEPTH_STENCIL_DESC desc{
			.DepthEnable{ TRUE },
			.DepthWriteMask{ D3D11_DEPTH_WRITE_MASK_ZERO },
			.DepthFunc{ D3D11_COMPARISON_GREATER },
			.StencilEnable{ FALSE }
		};

		hr = device->CreateDepthStencilState(&desc, &m_pTransDepthState);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pTransDepthState, "TransDepthState");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create sampler
	{
		hr = createSampler();
		if (FAILED(hr)) {
			return hr;
		}
	}

	m_pSphere = new Sphere(device, deviceContext);
	hr = m_pSphere->init();
	if (FAILED(hr)) {
		return hr;
	}

	m_pRect = new Rect(device, deviceContext);

	DirectX::XMFLOAT3 positions[]{
		{ 1.0f, 0, 0.0f },
		{ 1.2f, 0, 0.0f }
	};

	DirectX::XMFLOAT4 colors[]{
		{ 0.5f, 0, 0.5f, 1.0f },
		{ 0.5f, 0.5f, 0, 1.0f }
	};

	hr = m_pRect->init(positions, colors, 2);

	return hr;
}

void Renderer::termScene() {
	SAFE_RELEASE(viewProjectionBuffer);
	SAFE_RELEASE(rasterizerState);
	SAFE_RELEASE(sampler);

	SAFE_RELEASE(m_pDepthState);
	SAFE_RELEASE(m_pTransDepthState);

	SAFE_RELEASE(m_pTransBlendState);
	SAFE_RELEASE(m_pOpaqueBlendState);

	SAFE_RELEASE(m_pDepthBuffer);
	SAFE_RELEASE(m_pDepthBufferDSV);

	m_pCube->term();
	m_pSphere->term();
}

HRESULT Renderer::createViewProjectionBuffer() {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(ViewProjectionBuffer) },
		.Usage{ D3D11_USAGE_DYNAMIC },	// часто собираемся обновлять, храним ближе к CPU
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER },
		.CPUAccessFlags{ D3D11_CPU_ACCESS_WRITE }	// можем писать со стороны CPU  
	};

	return device->CreateBuffer(&desc, nullptr, &viewProjectionBuffer);
}

HRESULT Renderer::createRasterizerState() {
	D3D11_RASTERIZER_DESC desc{
		.FillMode{ D3D11_FILL_SOLID },
		.CullMode{ D3D11_CULL_NONE },
		.DepthClipEnable{ TRUE }
	};

	return device->CreateRasterizerState(&desc, &rasterizerState);
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

	return device->CreateSamplerState(&desc, &sampler);
}
