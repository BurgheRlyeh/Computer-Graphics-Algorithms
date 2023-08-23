#pragma once

#define NOMINMAX
#include <limits>

#include "framework.h"
#include "Texture.h"
#include "Camera.h"
#include "AABB.h"

struct TextureDesc;
class Renderer;
struct Camera;
struct AABB;

class Cube {
    typedef struct TextureTangentVertex {
        DirectX::XMFLOAT3 point{};
        DirectX::XMFLOAT3 tangent{};
        DirectX::XMFLOAT3 norm{};
        DirectX::XMFLOAT2 texture{};
    } Vertex;

    typedef struct ModelBuffer {
        DirectX::XMMATRIX matrix{};
        DirectX::XMMATRIX normalMatrix{};
        // x - shininess
        // y - rotation speed
        // z - texture id
        // w - normal map presence
        DirectX::XMFLOAT4 settings{};
        // x, y, z - position
        // w - current angle
        DirectX::XMFLOAT4 posAndAng{};
    } ModelBuffer;

    ID3D11Device* m_pDevice{};
    ID3D11DeviceContext* m_pDeviceContext{};

    ID3D11Buffer*       m_pVertexBuffer{};
    ID3D11Buffer*       m_pIndexBuffer{};
    
    ID3D11Buffer* m_pModelBufferInst{};
    ID3D11Buffer* m_pModelBufferInstVis{};
    std::vector<ModelBuffer> m_modelBuffers{ MaxInstances };
    std::vector<AABB> m_modelBBs{ MaxInstances };

    ID3D11VertexShader* m_pVertexShader{};
    ID3D11PixelShader*  m_pPixelShader{};
    ID3D11InputLayout*  m_pInputLayout{};

    ID3D11Texture2D* m_pTexture{};
    ID3D11ShaderResourceView* m_pTextureView{};

    ID3D11Texture2D* m_pTextureNM{};
    ID3D11ShaderResourceView* m_pTextureViewNM{};

    float m_modelRotationSpeed{ DirectX::XM_PIDIV2 };

public:
    static const int MaxInstances{ 100 };
    UINT m_instCount{ 2 };
    UINT m_instVisCount{ 0 };

    Cube(ID3D11Device* device, ID3D11DeviceContext* deviceContext):
        m_pDevice(device),
        m_pDeviceContext(deviceContext)
    {}

    bool m_doCull{ true };

    HRESULT init(DirectX::XMMATRIX* positions, int num);
    void term();

    void update(float delta, bool isRotate);
    void render(ID3D11SamplerState* sampler, ID3D11Buffer* viewProjectionBuffer);

    float getModelRotationSpeed();

    void initModel(ModelBuffer& modelBuffer, AABB& bb);

    void initImGUI();

    void cullBoxes(Camera& camera, float aspectRatio);

private:
    HRESULT createVertexBuffer(Vertex(&vertices)[], UINT numVertices);
    HRESULT createIndexBuffer(USHORT(&indices)[], UINT numIndices);
    HRESULT createModelBuffer();
    HRESULT createTexture(TextureDesc& textureDesc, ID3D11Texture2D** texture);
    HRESULT createResourceView(TextureDesc& textureDesc, ID3D11Texture2D* pTexture, ID3D11ShaderResourceView** pShaderResourceView);
};