#include "Rect.h"
#include "ShaderProcessor.h"

HRESULT Rect::init(DirectX::XMFLOAT3* positions, DirectX::XMFLOAT4* colors, int num) {
    m_pModelBuffers.resize(num);
    m_boundingRects.resize(num);
    
    HRESULT hr{ S_OK };

    Vertex vertices[]{
        { { 0.0, -0.75, -0.75 }, RGB(128,0,128)},
        { { 0.0,  0.75, -0.75 }, RGB(128,0,128)},
        { { 0.0,  0.75,  0.75 }, RGB(128,0,128)},
        { { 0.0, -0.75,  0.75 }, RGB(128,0,128)}
    };

    // create vertex buffer
    {

        hr = createVertexBuffer(vertices, sizeof(vertices) / sizeof(*vertices));
        if (FAILED(hr)) {
            return hr;
        }

        hr = SetResourceName(m_pVertexBuffer, "RectVertexBuffer");
        if (FAILED(hr)) {
            return hr;
        }
    }

    // create index buffer
    {
        UINT16 indices[]{
            0, 1, 2,
            0, 2, 3
        };

        hr = createIndexBuffer(indices, sizeof(indices) / sizeof(*indices));
        if (FAILED(hr)) {
            return hr;
        }

        hr = SetResourceName(m_pIndexBuffer, "RectIndexBuffer");
        if (FAILED(hr)) {
            return hr;
        }
    }

    // shaders processing
    ID3DBlob* vertexShaderCode{};
    {
        hr = compileAndCreateShader(
            m_pDevice,
            L"TransColor.vs",
            (ID3D11DeviceChild**)&m_pVertexShader,
            {},
            &vertexShaderCode
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = compileAndCreateShader(
            m_pDevice,
            L"TransColor.ps",
            (ID3D11DeviceChild**)&m_pPixelShader,
            { "USE_LIGHTS" }
        );
        if (FAILED(hr)) {
            return hr;
        }
    }

    // create input layout
    {
        D3D11_INPUT_ELEMENT_DESC InputDesc[]{
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                0,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "COLOR",
                0,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                0,
                12,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };

        hr = m_pDevice->CreateInputLayout(
            InputDesc,
            2,
            vertexShaderCode->GetBufferPointer(),
            vertexShaderCode->GetBufferSize(),
            &m_pInputLayout
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = SetResourceName(m_pInputLayout, "RectInputLayout");
        if (FAILED(hr)) {
            return hr;
        }
    }
    SAFE_RELEASE(vertexShaderCode);

    // create model buffers
    for (int idx{}; idx < num; ++idx) {
        ModelBuffer modelBuffer{
            DirectX::XMMatrixTranslation(
                positions[idx].x,
                positions[idx].y,
                positions[idx].z
            ),
            colors[idx]
        };

        hr = createModelBuffer(modelBuffer, idx);
        if (FAILED(hr)) {
            return hr;
        }

        hr = SetResourceName(m_pModelBuffers[idx], "RectModelBuffer" + idx);
        if (FAILED(hr)) {
            return hr;
        }

        // init bounding rects
        for (int i{}; i < sizeof(vertices) / sizeof(*vertices); ++i) {
            m_boundingRects[idx].v[i] = {
                vertices[i].point.x + positions[idx].x,
                vertices[i].point.y + positions[idx].y,
                vertices[i].point.z + positions[idx].z
            };
        }
    }

    return hr;
}

HRESULT Rect::createVertexBuffer(Vertex(&vertices)[], UINT numVertices) {
    D3D11_BUFFER_DESC desc{
        .ByteWidth{ sizeof(Vertex) * numVertices },
        .Usage{ D3D11_USAGE_IMMUTABLE },
        .BindFlags{ D3D11_BIND_VERTEX_BUFFER }
    };

    D3D11_SUBRESOURCE_DATA data{
        .pSysMem{ vertices },
        .SysMemPitch{ sizeof(Vertex) * numVertices }
    };

    return m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
}

HRESULT Rect::createIndexBuffer(USHORT(&indices)[], UINT numIndices) {
    D3D11_BUFFER_DESC desc{
        .ByteWidth{ sizeof(USHORT) * numIndices },
        .Usage{ D3D11_USAGE_IMMUTABLE },
        .BindFlags{ D3D11_BIND_INDEX_BUFFER }
    };

    D3D11_SUBRESOURCE_DATA data{
        .pSysMem{ indices },
        .SysMemPitch{ sizeof(USHORT) * numIndices }
    };

    return m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
}

HRESULT Rect::createModelBuffer(ModelBuffer& modelBuffer, int idx) {
    D3D11_BUFFER_DESC desc{
        .ByteWidth{ sizeof(ModelBuffer) },
        .Usage{ D3D11_USAGE_DEFAULT },				// храним в VRAM, изменяем
        .BindFlags{ D3D11_BIND_CONSTANT_BUFFER }	// константный
    };

    D3D11_SUBRESOURCE_DATA data{
        .pSysMem{ &modelBuffer },
        .SysMemPitch{ sizeof(modelBuffer) }
    };

    return m_pDevice->CreateBuffer(&desc, &data, &m_pModelBuffers[idx]);
}

void Rect::term() {
    SAFE_RELEASE(m_pVertexBuffer);
    SAFE_RELEASE(m_pIndexBuffer);
    for (auto pModelBuffer : m_pModelBuffers) {
        SAFE_RELEASE(pModelBuffer);
    }
    SAFE_RELEASE(m_pVertexShader);
    SAFE_RELEASE(m_pPixelShader);
    SAFE_RELEASE(m_pInputLayout);
}

void Rect::update(DirectX::XMMATRIX matrix) {
    ModelBuffer modelBuffer{ matrix };

    // обновление буфера
    m_pDeviceContext->UpdateSubresource(m_pModelBuffers[0], 0, nullptr, &modelBuffer, 0, 0);
}

void Rect::render(
    ID3D11SamplerState* sampler,
    ID3D11Buffer* viewProjectionBuffer,
    ID3D11DepthStencilState* m_pTransDepthState,
    ID3D11BlendState* m_pTransBlendState,
    DirectX::XMFLOAT3 cameraPos
) {
    m_pDeviceContext->OMSetDepthStencilState(m_pTransDepthState, 0);

    m_pDeviceContext->OMSetBlendState(m_pTransBlendState, nullptr, 0xFFFFFFFF);

    m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    ID3D11Buffer* cbuffers[] = { viewProjectionBuffer, nullptr };
    m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    m_pDeviceContext->IASetInputLayout(m_pInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
    m_pDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    std::vector<std::pair<float, int>> depthIdxPairs{};
    depthIdxPairs.resize(m_pModelBuffers.size());

    for (int i{}; i < m_pModelBuffers.size(); ++i) {
        float depth{};
        for (int j{}; j < 4; ++j) {
            DirectX::XMFLOAT3 point{
                cameraPos.x - m_boundingRects[i].v[j].x,
                cameraPos.y - m_boundingRects[i].v[j].y,
                cameraPos.z - m_boundingRects[i].v[j].z,
            };
            float length = point.x * point.x + point.y * point.y + point.z * point.z;

            depth = DirectX::XMMax(depth, length);
        }
        depthIdxPairs[i] = std::make_pair(depth, i);
    }

    std::sort(depthIdxPairs.begin(), depthIdxPairs.end());

    for (int i{ static_cast<int>(m_pModelBuffers.size() - 1) }; 0 <= i; --i) {
        cbuffers[1] = m_pModelBuffers[depthIdxPairs[i].second];
        m_pDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
        m_pDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
        m_pDeviceContext->DrawIndexed(6, 0, 0);
    }
}
