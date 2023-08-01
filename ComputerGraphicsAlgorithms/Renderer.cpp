#include "framework.h"
#include "Renderer.h"

#include <chrono>

#define NOMINMAX

bool Renderer::init(HWND hWnd) {
	// Create a DirectX graphics interface factory.
	IDXGIFactory* factory{};
	HRESULT hr{ CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory) };
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
		.poi{ DirectX::XMFLOAT3(0, 0, 0) },
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

void Renderer::term() {
	SAFE_RELEASE(backBufferRTV);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(deviceContext);
	SAFE_RELEASE(device);
}

bool Renderer::update() {
	size_t usec{ static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now().time_since_epoch()
	).count()) };

	if (!prevUSec) {
		prevUSec = usec;
	}

	if (isModelRotate) {
		angle += modelRotationSpeed * (usec - prevUSec) / 1e6; 	// обновление угла вращения

		ModelBuffer geometryBuffer{
			// матрица вращения вокруг оси
			DirectX::XMMatrixRotationAxis(
				// вектор, описывающий ось вращения
				DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
				static_cast<float>(angle)
			) 
		};

		// обновление буфера
		deviceContext->UpdateSubresource(this->modelBuffer, 0, nullptr, &geometryBuffer, 0, 0);
	}

	prevUSec = usec;

	// Setup camera
	DirectX::XMMATRIX v{
		// создание матрицы для системы координат левой руки
		DirectX::XMMatrixLookAtLH(
			// позиция камеры
			DirectX::XMVectorSet(
				camera.poi.x + camera.r * cosf(camera.angY) * cosf(camera.angZ),
				camera.poi.y + camera.r * sinf(camera.angY),
				camera.poi.z + camera.r * cosf(camera.angY) * sinf(camera.angZ),
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
			2 * nearPlane * tanf(fov / 2),					// Ширина усеченного конуса в nearPlane
			2 * nearPlane * tanf(fov / 2) * aspectRatio,	// Высота усеченного конуса в nearPlane
			nearPlane,										// Расстояние до ближайшей плоскости отсечения
			farPlane										// Расстояние до дальней плоскости отсечения
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
	deviceContext->Unmap(this->viewProjectionBuffer, 0);

	return SUCCEEDED(hr);
}

bool Renderer::render() {
	deviceContext->ClearState();

	ID3D11RenderTargetView* views[]{ backBufferRTV };
	deviceContext->OMSetRenderTargets(1, views, nullptr);

	static const FLOAT BackColor[4]{ 0.25f, 0.25f, 0.25f, 1.0f };
	deviceContext->ClearRenderTargetView(backBufferRTV, BackColor);

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

	//deviceContext->RSSetState(rasterizerState);

	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	ID3D11Buffer* vertexBuffers[]{ vertexBuffer };
	UINT strides[]{ 16 };
	UINT offsets[]{ 0 };
	ID3D11Buffer* cbuffers[]{ viewProjectionBuffer, modelBuffer };
	deviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	deviceContext->IASetInputLayout(inputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 2, cbuffers);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->DrawIndexed(36, 0, 0);

	return SUCCEEDED(swapChain->Present(0, 0));
}

bool Renderer::resize(UINT width, UINT height) {
	if (this->width == width && this->height == height) {
		return true;
	}

	SAFE_RELEASE(backBufferRTV);

	HRESULT hr{ swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0) };
	if (FAILED(hr)) {
		return false;
	}

	this->width = width;
	this->height = height;

	return SUCCEEDED(setupBackBuffer());
}

void Renderer::mouseRBPressed(bool isPressed, int x, int y) {
	isMRBPressed = isPressed;
	if (!isMRBPressed) {
		return;
	}

	prevMouseX = x;
	prevMouseY = y;
}

void Renderer::mouseMoved(int x, int y) {
	if (!isMRBPressed) {
		return;
	}

	camera.angZ += (float)(x - prevMouseX) / width * cameraRotationSpeed;
	camera.angY += (float)(y - prevMouseY) / width * cameraRotationSpeed;
	camera.angY = DirectX::XMMax(camera.angY, -modelRotationSpeed);
	camera.angY = DirectX::XMMin(camera.angY, modelRotationSpeed);

	prevMouseX = x;
	prevMouseY = y;
}

void Renderer::mouseWheel(int delta) {
	camera.r = DirectX::XMMax(camera.r - delta / 100.0f, 1.0f);
}

void Renderer::keyPressed(int keyCode) {
	if (keyCode == ' ') {
		isModelRotate = !isModelRotate;
	}
}

HRESULT Renderer::setupBackBuffer() {
	ID3D11Texture2D* backBuffer{};
	HRESULT hr{ swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer) };
	if (FAILED(hr)) {
		return hr;
	}

	hr = device->CreateRenderTargetView(backBuffer, nullptr, &backBufferRTV);
	SAFE_RELEASE(backBuffer);

	return hr;
}

HRESULT Renderer::initScene() {
	HRESULT hr{ S_OK };

	// create vertex buffer
	{
		Vertex vertices[24]{
			// Bottom
			{ { -0.5, -0.5,  0.5 }, RGB(0, 0, 0) },
			{ {  0.5, -0.5,  0.5 }, RGB(0, 0, 255) },
			{ {  0.5, -0.5, -0.5 }, RGB(0, 255, 0) },
			{ { -0.5, -0.5, -0.5 }, RGB(0, 255, 255) },
			// Top
			{ { -0.5,  0.5, -0.5 }, RGB(255, 0, 0) },
			{ {  0.5,  0.5, -0.5 }, RGB(255, 0, 255) },
			{ {  0.5,  0.5,  0.5 }, RGB(255, 255, 0) },
			{ { -0.5,  0.5,  0.5 }, RGB(255, 255, 255) }
		};

		hr = createVertexBuffer(vertices, sizeof(vertices) / sizeof(*vertices));
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(vertexBuffer, "VertexBuffer");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create index buffer
	{
		USHORT indices[36]{
			0, 2, 1, 0, 1, 7,
			0, 3, 2, 0, 4, 3,
			0, 7, 4, 1, 2, 6,
			1, 6, 7, 2, 3, 5,
			2, 5, 6, 3, 4, 5,
			4, 6, 5, 4, 7, 6
		};

		hr = createIndexBuffer(indices, sizeof(indices) / sizeof(*indices));
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(indexBuffer, "IndexBuffer");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// shaders processing
	ID3DBlob* vertexShaderCode{};
	{
		hr = compileAndCreateShader(L"VertexShader.vs", (ID3D11DeviceChild**)&vertexShader, &vertexShaderCode);
		if (FAILED(hr)) {
			return hr;
		}

		hr = compileAndCreateShader(L"PixelShader.ps", (ID3D11DeviceChild**)&pixelShader);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create input layout
	{
		D3D11_INPUT_ELEMENT_DESC InputDesc[]{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		hr = device->CreateInputLayout(
			InputDesc,
			2,
			vertexShaderCode->GetBufferPointer(),
			vertexShaderCode->GetBufferSize(),
			&inputLayout
		);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(inputLayout, "InputLayout");
		if (FAILED(hr)) {
			return hr;
		}
	}
	SAFE_RELEASE(vertexShaderCode);

	// create model buffer
	{
		ModelBuffer modelBuffer{ DirectX::XMMatrixIdentity() };

		hr = createModelBuffer(modelBuffer);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(this->modelBuffer, "GeomBuffer");
		if (FAILED(hr)) {
			return hr;
		}
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

	return hr;
}

void Renderer::termScene() {
	SAFE_RELEASE(rasterizerState);

	SAFE_RELEASE(inputLayout);
	SAFE_RELEASE(pixelShader);
	SAFE_RELEASE(vertexBuffer);

	SAFE_RELEASE(indexBuffer);
	SAFE_RELEASE(vertexBuffer);

	SAFE_RELEASE(viewProjectionBuffer);
	SAFE_RELEASE(modelBuffer);
}

HRESULT Renderer::createVertexBuffer(Vertex(&vertices)[], UINT numVertices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(Vertex)* numVertices },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_VERTEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ vertices },
		.SysMemPitch{ sizeof(Vertex)* numVertices }
	};

	return device->CreateBuffer(&desc, &data, &vertexBuffer);
}

HRESULT Renderer::createIndexBuffer(USHORT(&indices)[], UINT numIndices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(USHORT)* numIndices },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_INDEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ indices },
		.SysMemPitch{ sizeof(USHORT)* numIndices }
	};

	return device->CreateBuffer(&desc, &data, &indexBuffer);
}

HRESULT Renderer::createModelBuffer(ModelBuffer& modelBuffer) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(ModelBuffer) },
		.Usage{ D3D11_USAGE_DEFAULT },				// храним в VRAM, изменяем
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }	// константный
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ &modelBuffer },
		.SysMemPitch{ sizeof(modelBuffer) }
	};

	return device->CreateBuffer(&desc, &data, &this->modelBuffer);
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
		.CullMode{ D3D11_CULL_NONE },	// draw all sides
		.DepthClipEnable{ TRUE }
	};

	return device->CreateRasterizerState(&desc, &rasterizerState);
}

HRESULT Renderer::compileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode) {
	// Try to load shader's source code first
	FILE* pFile{};
	_wfopen_s(&pFile, path.c_str(), L"rb");
	if (!pFile) {
		return E_FAIL;
	}

	fseek(pFile, 0, SEEK_END);
	long long size = _ftelli64(pFile);
	fseek(pFile, 0, SEEK_SET);

	std::vector<char> data;
	data.resize(size + 1);

	size_t rd{ fread(data.data(), 1, size, pFile) };
	assert(rd == (size_t)size);

	fclose(pFile);

	// Determine shader's type
	std::wstring ext{ Extension(path) };

	std::string entryPoint{ WCSToMBS(ext) };
	std::string platform{ entryPoint + "_5_0"};

	// Setup flags
	UINT flags{};
#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

	// Try to compile
	ID3DBlob* pCode{};
	ID3DBlob* pErrMsg{};
	HRESULT hr{ D3DCompile(
		data.data(),
		data.size(),
		WCSToMBS(path).c_str(),
		nullptr,
		nullptr,
		entryPoint.c_str(),
		platform.c_str(),
		flags,
		0,
		&pCode,
		&pErrMsg
	) };

	if (FAILED(hr)) {
		if (pErrMsg) {
			OutputDebugStringA(static_cast<const char*>(pErrMsg->GetBufferPointer()));
		}
		return hr;
	}
	SAFE_RELEASE(pErrMsg);

	// Create shader itself if anything else is OK
	if (ext == L"vs") {
		ID3D11VertexShader* pVertexShader{};
		hr = device->CreateVertexShader(
			pCode->GetBufferPointer(),
			pCode->GetBufferSize(),
			nullptr,
			&pVertexShader
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppShader = pVertexShader;
	} else if (ext == L"ps") {
		ID3D11PixelShader* pPixelShader{};
		hr = device->CreatePixelShader(
			pCode->GetBufferPointer(),
			pCode->GetBufferSize(),
			nullptr,
			&pPixelShader
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppShader = pPixelShader;
	}

	hr = SetResourceName(*ppShader, WCSToMBS(path).c_str());

	if (ppCode) {
		*ppCode = pCode;
	} else {
		SAFE_RELEASE(pCode);
	}

	return hr;
}
