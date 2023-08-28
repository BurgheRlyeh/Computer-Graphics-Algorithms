#pragma once

#include "framework.h"
#include "Texture.h"

struct TextureDesc;

class Sphere {
    typedef struct ModelBuffer {
        DirectX::SimpleMath::Matrix matrix;
        DirectX::SimpleMath::Vector4 size;
    } ModelBuffer;

    ID3D11Device* m_pDevice{};
    ID3D11DeviceContext* m_pDeviceContext{};

    UINT m_indexCount{};
    ID3D11Buffer* m_pVertexBuffer{};
    ID3D11Buffer* m_pIndexBuffer{};
    ID3D11Buffer* m_pModelBuffer{};
    ID3D11VertexShader* m_pVertexShader{};
    ID3D11PixelShader* m_pPixelShader{};
    ID3D11InputLayout* m_pInputLayout{};

    ID3D11Texture2D* m_pCubeMapTexture{};
    ID3D11ShaderResourceView* m_pCubeMapView{};

public:
    Sphere(ID3D11Device* device, ID3D11DeviceContext* deviceContext):
        m_pDevice(device),
        m_pDeviceContext(deviceContext)
    {}

    HRESULT init();
    void term();

    void resize(float r);
    void render(ID3D11SamplerState* sampler, ID3D11Buffer* viewProjectionBuffer);

private:
    HRESULT createVertexBuffer(std::vector<DirectX::SimpleMath::Vector3>& vertices);
    HRESULT createIndexBuffer(std::vector<UINT16>& indices);
    HRESULT createModelBuffer();
    HRESULT createTexture(TextureDesc(&texDescs)[]);
    HRESULT createResourceView(TextureDesc(&texDescs)[]);
};
