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

    renderer.m_camera.angZ += -(float)(x - prevMouseX) / renderer.m_width * renderer.m_camera.cameraRotationSpeed;
    renderer.m_camera.angY += (float)(y - prevMouseY) / renderer.m_width * renderer.m_camera.cameraRotationSpeed;
    renderer.m_camera.angY = DirectX::XMMax(renderer.m_camera.angY, -renderer.m_pCube->getModelRotationSpeed());
    renderer.m_camera.angY = DirectX::XMMin(renderer.m_camera.angY, renderer.m_pCube->getModelRotationSpeed());

    prevMouseX = x;
    prevMouseY = y;
}

void Renderer::MouseHandler::mouseWheel(int delta) {
    renderer.m_camera.r = DirectX::XMMax(renderer.m_camera.r - delta / 100.0f, 1.0f);
}

void Renderer::KeyboardHandler::keyPressed(int keyCode) {
    switch (keyCode) {
        case ' ':
            renderer.m_isModelRotate = !renderer.m_isModelRotate;
            break;

        case 'W':
        case 'w':
            renderer.m_camera.forwardDelta -= panSpeed;
            break;

        case 'S':
        case 's':
            renderer.m_camera.forwardDelta += panSpeed;
            break;

        case 'D':
        case 'd':
            renderer.m_camera.rightDelta -= panSpeed;
            break;

        case 'A':
        case 'a':
            renderer.m_camera.rightDelta += panSpeed;
            break;
    }
}

void Renderer::KeyboardHandler::keyReleased(int keyCode) {
    switch (keyCode) {
        case 'W':
        case 'w':
            renderer.m_camera.forwardDelta += panSpeed;
            break;

        case 'S':
        case 's':
            renderer.m_camera.forwardDelta -= panSpeed;
            break;

        case 'D':
        case 'd':
            renderer.m_camera.rightDelta += panSpeed;
            break;

        case 'A':
        case 'a':
            renderer.m_camera.rightDelta -= panSpeed;
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
    m_camera = Camera{
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
    SAFE_RELEASE(m_pBackBufferRTV);
    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pDeviceContext);

#ifdef _DEBUG
    if (m_pDevice) {
        ID3D11Debug* debug{};
        HRESULT hr{ m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug) };
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
    SAFE_RELEASE(m_pDepthBuffer);
    SAFE_RELEASE(m_pDepthBufferDSV);

    HRESULT hr{ m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0) };
    if (FAILED(hr)) {
        return false;
    }

    this->m_width = width;
    this->m_height = height;

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
    size_t time{ static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count()) };

    if (!m_prevTime) {
        m_prevTime = time;
    }

    // move camera
    m_camera.move((time - m_prevTime) / 1e6f);

    if (m_isModelRotate) {

        m_cubeAngleRotation += m_pCube->getModelRotationSpeed() * (time - m_prevTime) / 1e6f;

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

    m_prevTime = time;

    DirectX::XMFLOAT3 cameraPos{
        m_camera.poi.x + m_camera.r * cosf(m_camera.angY) * cosf(m_camera.angZ),
        m_camera.poi.y + m_camera.r * sinf(m_camera.angY),
        m_camera.poi.z + m_camera.r * cosf(m_camera.angY) * sinf(m_camera.angZ)
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
                m_camera.poi.x,
                m_camera.poi.y,
                m_camera.poi.z,
                0.0f
            ),
            // направление вверх от камеры
            DirectX::XMVectorSet(
                cosf(m_camera.angY + DirectX::XM_PIDIV2) * cosf(m_camera.angZ),
                sinf(m_camera.angY + DirectX::XM_PIDIV2),
                cosf(m_camera.angY + DirectX::XM_PIDIV2) * sinf(m_camera.angZ),
                0.0f
            )
        )
    };

    float nearPlane{ 0.1f };
    float farPlane{ 100.0f };
    float fov{ DirectX::XM_PI / 3 };
    float aspectRatio{ static_cast<float>(m_height) / m_width };

    DirectX::XMMATRIX p{
        // Матрица построения перспективы для левой руки
        DirectX::XMMatrixPerspectiveLH(
            2 * farPlane * tanf(fov / 2), // width
            2 * farPlane * tanf(fov / 2) * aspectRatio,	// height
            farPlane, // rho to near
            nearPlane // rho to far
        )
    };

    D3D11_MAPPED_SUBRESOURCE subresource;
    HRESULT hr = m_pDeviceContext->Map(
        m_pVPBuffer, // указатель на ресурс
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
    m_pDeviceContext->Unmap(this->m_pVPBuffer, 0);

    return SUCCEEDED(hr);
}

bool Renderer::render() {
    m_pDeviceContext->ClearState();

    ID3D11RenderTargetView* views[]{ m_pBackBufferRTV };
    // привязываем буфер глубины к Output-Merger этапу
    m_pDeviceContext->OMSetRenderTargets(1, views, m_pDepthBufferDSV);

    static const FLOAT BackColor[4]{ 0.25f, 0.25f, 0.25f, 1.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);
    // очистка буфера глубины
    m_pDeviceContext->ClearDepthStencilView(m_pDepthBufferDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);

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

    m_pSphere->render(m_pSampler, m_pVPBuffer);

    m_pCube->render(m_pSampler, m_pVPBuffer);

    DirectX::XMFLOAT3 cameraPos{
        m_camera.poi.x + m_camera.r * cosf(m_camera.angY) * cosf(m_camera.angZ),
        m_camera.poi.y + m_camera.r * sinf(m_camera.angY),
        m_camera.poi.z + m_camera.r * cosf(m_camera.angY) * sinf(m_camera.angZ)
    };

    m_pRect->render(
        m_pSampler,
        m_pVPBuffer,
        m_pTransDepthState,
        m_pTransBlendState,
        cameraPos
    );

    return SUCCEEDED(m_pSwapChain->Present(0, 0));
}

HRESULT Renderer::setupBackBuffer() {
    ID3D11Texture2D* backBuffer{};
    HRESULT hr{ m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer) };
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pDevice->CreateRenderTargetView(backBuffer, nullptr, &m_pBackBufferRTV);
    SAFE_RELEASE(backBuffer);
    if (FAILED(hr)) {
        return hr;
    }

    hr = createDepthBuffer();
    if (FAILED(hr)) {
        return hr;
    }

    hr = SetResourceName(m_pDepthBuffer, "DepthBuffer");
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pDevice->CreateDepthStencilView(
        m_pDepthBuffer,
        nullptr,
        &m_pDepthBufferDSV
    );
    if (FAILED(hr)) {
        return hr;
    }

    hr = SetResourceName(m_pDepthBufferDSV, "DepthBufferView");
    if (FAILED(hr)) {
        return hr;
    }


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
    
    return m_pDevice->CreateTexture2D(&desc, nullptr, &m_pDepthBuffer);
}

HRESULT Renderer::initScene() {
    HRESULT hr{ S_OK };

    m_pCube = new Cube(m_pDevice, m_pDeviceContext);

    DirectX::XMMATRIX positions[]{
        DirectX::XMMatrixIdentity(),
        DirectX::XMMatrixTranslation(2.0f, 0.0f, 0.0f)
    };

    hr = m_pCube->init(positions, 2);
    if (FAILED(hr)) {
        return hr;
    }

    // create view-projection buffer
    {
        hr = createViewProjectionBuffer();
        if (FAILED(hr)) { 
            return hr;
        }

        hr = SetResourceName(m_pVPBuffer, "SceneBuffer");
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

        hr = SetResourceName(m_pRasterizerState, "RasterizerState");
        if (FAILED(hr)) {
            return hr;
        }
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
        if (FAILED(hr)) {
            return hr;
        }

        hr = SetResourceName(m_pTransBlendState, "TransBlendState");
        if (FAILED(hr)) {
            return hr;
        }

        desc.RenderTarget[0].BlendEnable = FALSE;
        hr = m_pDevice->CreateBlendState(&desc, &m_pOpaqueBlendState);
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
        hr = createReversedDepthState();
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

        hr = m_pDevice->CreateDepthStencilState(&desc, &m_pTransDepthState);
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

    m_pSphere = new Sphere(m_pDevice, m_pDeviceContext);
    hr = m_pSphere->init();
    if (FAILED(hr)) {
        return hr;
    }

    m_pRect = new Rect(m_pDevice, m_pDeviceContext);

    DirectX::XMFLOAT3 rectPositions[]{
        { 1.0f, 0, 0.0f },
        { 1.2f, 0, 0.0f }
    };

    DirectX::XMFLOAT4 rectColors[]{
        { 1.0f, 0.25f, 0.0f, 1.0f },
        { 0.0f, 0.25f, 1.0f, 1.0f }
    };

    hr = m_pRect->init(rectPositions, rectColors, 2);

    return hr;
}

void Renderer::termScene() {
    SAFE_RELEASE(m_pVPBuffer);
    SAFE_RELEASE(m_pRasterizerState);
    SAFE_RELEASE(m_pSampler);

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

    return m_pDevice->CreateBuffer(&desc, nullptr, &m_pVPBuffer);
}

HRESULT Renderer::createRasterizerState() {
    D3D11_RASTERIZER_DESC desc{
        .FillMode{ D3D11_FILL_SOLID },
        .CullMode{ D3D11_CULL_NONE },
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
