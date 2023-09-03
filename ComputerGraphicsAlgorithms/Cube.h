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
public:
    static const int MaxInstances{ 100 };

private:
    typedef struct TextureTangentVertex {
        DirectX::SimpleMath::Vector3 point{};
        DirectX::SimpleMath::Vector3 tangent{};
        DirectX::SimpleMath::Vector3 norm{};
        DirectX::SimpleMath::Vector2 texture{};
    } Vertex;

    typedef struct ModelBuffer {
        DirectX::SimpleMath::Matrix matrix{};
        DirectX::SimpleMath::Matrix normalMatrix{};
        // x - shininess
        // y - rotation speed
        // z - texture id
        // w - normal map presence
        DirectX::SimpleMath::Vector4 settings{};
        // x, y, z - position
        // w - current angle
        DirectX::SimpleMath::Vector4 posAndAng{};

        void updateMatrices();
    } ModelBuffer;

    struct CullParams {
        DirectX::XMINT4 shapeCount; // x - shapes count
        DirectX::SimpleMath::Vector4 bbMin[MaxInstances]{};
        DirectX::SimpleMath::Vector4 bbMax[MaxInstances]{};
    };

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

    ID3D11ComputeShader* m_pCullShader{};
    ID3D11Buffer* m_pIndirectArgsSrc{};
    ID3D11Buffer* m_pIndirectArgs{};
    ID3D11Buffer* m_pCullParams{};
    ID3D11Buffer* m_pModelBufferInstVisGPU{};
    ID3D11UnorderedAccessView* m_pModelBufferInstVisGPU_UAV{};
    ID3D11UnorderedAccessView* m_pIndirectArgsUAV{};
    ID3D11Query* m_queries[10]{};
    UINT64 m_curFrame{};
    UINT64 m_lastCompletedFrame{};
    bool m_computeCull{};
    bool m_updateCullParams{};
    int m_gpuVisibleInstances{};

    UINT m_instCount{ 2 };
    UINT m_instVisCount{ 0 };

    bool m_doCull{ true };

public:
    Cube() = delete;
    Cube(ID3D11Device* device, ID3D11DeviceContext* deviceContext):
        m_pDevice(device),
        m_pDeviceContext(deviceContext)
    {}

    float getModelRotationSpeed();
    bool getIsDoCull();

    HRESULT init(DirectX::SimpleMath::Matrix* positions, int num);
    void term();

    void update(float delta, bool isRotate);
    void render(ID3D11SamplerState* sampler, ID3D11Buffer* viewProjectionBuffer);

    HRESULT initCull();
    void calcFrustum(Camera& camera, float aspectRatio, DirectX::SimpleMath::Plane frustum[6]);
    void updateCullParams();
    void cullBoxes(ID3D11Buffer* m_pSceneBuffer, Camera& camera, float aspectRatio);
    void readQueries();

    void initImGUI();

private:
    HRESULT createVertexBuffer(Vertex(&vertices)[], UINT numVertices);
    HRESULT createIndexBuffer(USHORT(&indices)[], UINT numIndices);
    HRESULT createModelBuffer();
    HRESULT createTexture(TextureDesc& textureDesc, ID3D11Texture2D** texture);
    HRESULT createResourceView(TextureDesc& textureDesc, ID3D11Texture2D* pTexture, ID3D11ShaderResourceView** pShaderResourceView);

    void initModel(ModelBuffer& modelBuffer, AABB& bb);

};

//struct Light {
//    float4 pos;
//    float4 color;
//};

//cbuffer SceneBuffer: register(b0) {
//    float4x4 vp;
//    float4 cameraPos;
//    int4 lightCount;
//    int4 postProcess;
//    Light lights[10];
//    float4 ambientColor;
//    float4 frustum[6];  // frustum planes
//};

//cbuffer CullParams: register(b1) {
//    uint4 numShapes; // x - objects count
//    float4 bbMin[100];  // aabb min points
//    float4 bbMax[100];  // aabb max points
//};

//// 
//RWStructuredBuffer<uint> indirectArgs: register(u0);

//// индексы моделей для рисования
//RWStructuredBuffer<uint4> objectIds: register(u1);

//bool IsBoxInside(in float4 frustum[6], in float3 bbMin, in float3 bbMax) {
//    for (int i = 0; i < 6; i++) {
//        float4 p = float4(
//            frustum[i].x < 0 ? bbMin.x : bbMax.x,
//            frustum[i].y < 0 ? bbMin.y : bbMax.y,
//            frustum[i].z < 0 ? bbMin.z : bbMax.z,
//            1.0
//        );
//        if (dot(p, frustum[i]) < 0.0f) {
//            return false;
//        }
//    }

//    return true;
//}

//[numthreads(64, 1, 1)]  // размер группы
//void cs(uint3 globalThreadId: SV_DispatchThreadID) {
//    if (globalThreadId.x >= numShapes.x) {
//        return;
//    }

//    if (IsBoxInside(frustum, bbMin[globalThreadId.x].xyz, bbMax[globalThreadId.x].xyz)) {
//        uint id = 0;
//        InterlockedAdd(indirectArgs[1], 1, id); // ++indirectArgs
//        objectIds[id] = uint4(globalThreadId.x, 0, 0, 0);
//    }
//}
