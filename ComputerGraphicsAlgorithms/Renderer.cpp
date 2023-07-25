#include "framework.h"

#include "Renderer.h"

bool Renderer::Init(HWND hWnd) {
	// Create a DirectX graphics interface factory.
	IDXGIFactory* factory{};
	auto result{ CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory) };


	if (SUCCEEDED(result)) {
		IDXGIAdapter* adapter{ selectIDXGIAdapter(factory) };
		assert(adapter);

		result = createDeviceAndSwapChain(hWnd, adapter);
		assert(SUCCEEDED(result));

		SAFE_RELEASE(adapter);
	}

	if (SUCCEEDED(result)) {
		result = SetupBackBuffer();
	}

	SAFE_RELEASE(factory);

	return SUCCEEDED(result);
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

	HRESULT result{ D3D11CreateDeviceAndSwapChain(
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

	return result;
}

void Renderer::Term() {
	SAFE_RELEASE(backBufferRTV);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(deviceContext);
	SAFE_RELEASE(device);
}

bool Renderer::Render() {
	deviceContext->ClearState();

	ID3D11RenderTargetView* views[]{ backBufferRTV };
	deviceContext->OMSetRenderTargets(1, views, nullptr);

	static const FLOAT BackColor[4]{ 0.25f, 0.25f, 0.25f, 1.0f };
	deviceContext->ClearRenderTargetView(backBufferRTV, BackColor);

	HRESULT result{ swapChain->Present(0, 0) };
	assert(SUCCEEDED(result));

	return SUCCEEDED(result);
}

bool Renderer::Resize(UINT width, UINT height) {
	if (this->width == width && this->height == height) {
		return true;
	}

	SAFE_RELEASE(backBufferRTV);

	HRESULT result{ swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0) };
	assert(SUCCEEDED(result));
	if (SUCCEEDED(result)) {
		this->width = width;
		this->height = height;

		result = SetupBackBuffer();
	}

	return SUCCEEDED(result);
}

HRESULT Renderer::SetupBackBuffer() {
	ID3D11Texture2D* pBackBuffer{};
	HRESULT result{ swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer) };
	assert(SUCCEEDED(result));

	if (SUCCEEDED(result)) {
		result = device->CreateRenderTargetView(pBackBuffer, NULL, &backBufferRTV);
		assert(SUCCEEDED(result));

		SAFE_RELEASE(pBackBuffer);
	}

	return result;
}
