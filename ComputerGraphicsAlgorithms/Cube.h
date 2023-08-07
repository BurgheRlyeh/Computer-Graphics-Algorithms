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
    } CubeModelBuffer;

    ID3D11Device* m_pDevice{};
    ID3D11DeviceContext* m_pDeviceContext{};

    // cube
    ID3D11Buffer*       m_pVertexBuffer{};
    ID3D11Buffer*       m_pIndexBuffer{};
    ID3D11Buffer*       m_pModelBuffer{};
    ID3D11VertexShader* m_pVertexShader{};
    ID3D11PixelShader*  m_pPixelShader{};
    ID3D11InputLayout*  m_pInputLayout{};

    ID3D11Texture2D* m_pTexture{};
    ID3D11ShaderResourceView* m_pTextureView{};

public:
    Cube(ID3D11Device* device, ID3D11DeviceContext* deviceContext):
        m_pDevice(device),
        m_pDeviceContext(deviceContext)
    {}

    ID3D11Buffer* getVertexBuffer();
    ID3D11Buffer* getIndexBuffer();
    ID3D11Buffer* getModelBuffer();
    ID3D11VertexShader* getVertexShader();
    ID3D11PixelShader* getPixelShader();
    ID3D11InputLayout* getInputLayout();

    ID3D11Texture2D* getTexture();
    ID3D11ShaderResourceView* getTextureView();

    HRESULT init();
    void term();

    void update(float angle);
    void render(ID3D11SamplerState* sampler, ID3D11Buffer* viewProjectionBuffer);

private:
    HRESULT createVertexBuffer(Vertex(&vertices)[], UINT numVertices);
    HRESULT createIndexBuffer(USHORT(&indices)[], UINT numIndices);
    HRESULT createModelBuffer(CubeModelBuffer& modelBuffer);
    HRESULT createTexture(TextureDesc& textureDesc);
    HRESULT createResourceView(TextureDesc& textureDesc);
};