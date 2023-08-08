#pragma once

#include "framework.h"
#include "Texture.h"

struct TextureDesc;

class Cube {
    typedef struct Vertex {
        DirectX::XMFLOAT3 point;
        DirectX::XMFLOAT2 texture;
    } Vertex;

    typedef struct ModelBuffer {
        DirectX::XMMATRIX m;
    } ModelBuffer;

    ID3D11Device* m_pDevice{};
    ID3D11DeviceContext* m_pDeviceContext{};

    ID3D11Buffer*       m_pVertexBuffer{};
    ID3D11Buffer*       m_pIndexBuffer{};
    std::vector<ID3D11Buffer*> m_pModelBuffers{};
    ID3D11VertexShader* m_pVertexShader{};
    ID3D11PixelShader*  m_pPixelShader{};
    ID3D11InputLayout*  m_pInputLayout{};

    ID3D11Texture2D* m_pTexture{};
    ID3D11ShaderResourceView* m_pTextureView{};

    float m_modelRotationSpeed{ DirectX::XM_PIDIV2 };

public:
    Cube(ID3D11Device* device, ID3D11DeviceContext* deviceContext):
        m_pDevice(device),
        m_pDeviceContext(deviceContext)
    {}

    HRESULT init(int num);
    void term();

    void update(int idx, DirectX::XMMATRIX matrix);
    void render(ID3D11SamplerState* sampler, ID3D11Buffer* viewProjectionBuffer);

    float getModelRotationSpeed();

private:
    HRESULT createVertexBuffer(Vertex(&vertices)[], UINT numVertices);
    HRESULT createIndexBuffer(USHORT(&indices)[], UINT numIndices);
    HRESULT createModelBuffer(ModelBuffer& modelBuffer, int idx);
    HRESULT createTexture(TextureDesc& textureDesc);
    HRESULT createResourceView(TextureDesc& textureDesc);
};