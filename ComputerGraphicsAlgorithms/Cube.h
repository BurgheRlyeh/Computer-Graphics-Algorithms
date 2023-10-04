#pragma once

#define NOMINMAX
#include <limits>

#include "framework.h"
#include "Texture.h"
#include "Camera.h"
#include "AABB.h"
#include "Timer.h"

struct TextureDesc;
class Renderer;
struct Camera;
struct AABB;
class CPUTimer;
class GPUTimer;

class Cube {
public:
	static const int MaxInstances{ 50 };

public:
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

	struct ShaderVertex {
		DirectX::SimpleMath::Vector4 point{};
		DirectX::SimpleMath::Vector4 tangent{};
		DirectX::SimpleMath::Vector4 norm{};
		DirectX::SimpleMath::Vector4 texture{};
	};

	struct VIBuffer {
		ShaderVertex vertices[24]{
			// Bottom face
			{ { -0.5, -0.5,  0.5, 1.f }, {  1,  0,  0, 0 }, {  0, -1,  0, 0 }, { 0, 1, 0, 0 } },
			{ {  0.5, -0.5,  0.5, 1.f }, {  1,  0,  0, 0 }, {  0, -1,  0, 0 }, { 1, 1, 0, 0 } },
			{ {  0.5, -0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0, -1,  0, 0 }, { 1, 0, 0, 0 } },
			{ { -0.5, -0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0, -1,  0, 0 }, { 0, 0, 0, 0 } },
			// Front face 
			{ { -0.5,  0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  1,  0, 0 }, { 0, 1, 0, 0 } },
			{ {  0.5,  0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  1,  0, 0 }, { 1, 1, 0, 0 } },
			{ {  0.5,  0.5,  0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  1,  0, 0 }, { 1, 0, 0, 0 } },
			{ { -0.5,  0.5,  0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  1,  0, 0 }, { 0, 0, 0, 0 } },
			// Top face
			{ {  0.5, -0.5, -0.5, 1.f }, {  0,  0,  1, 0 }, {  1,  0,  0, 0 }, { 0, 1, 0, 0 } },
			{ {  0.5, -0.5,  0.5, 1.f }, {  0,  0,  1, 0 }, {  1,  0,  0, 0 }, { 1, 1, 0, 0 } },
			{ {  0.5,  0.5,  0.5, 1.f }, {  0,  0,  1, 0 }, {  1,  0,  0, 0 }, { 1, 0, 0, 0 } },
			{ {  0.5,  0.5, -0.5, 1.f }, {  0,  0,  1, 0 }, {  1,  0,  0, 0 }, { 0, 0, 0, 0 } },
			// Back face
			{ { -0.5, -0.5,  0.5, 1.f }, {  0,  0, -1, 0 }, { -1,  0,  0, 0 }, { 0, 1, 0, 0 } },
			{ { -0.5, -0.5, -0.5, 1.f }, {  0,  0, -1, 0 }, { -1,  0,  0, 0 }, { 1, 1, 0, 0 } },
			{ { -0.5,  0.5, -0.5, 1.f }, {  0,  0, -1, 0 }, { -1,  0,  0, 0 }, { 1, 0, 0, 0 } },
			{ { -0.5,  0.5,  0.5, 1.f }, {  0,  0, -1, 0 }, { -1,  0,  0, 0 }, { 0, 0, 0, 0 } },
			// Right face
			{ {  0.5, -0.5,  0.5, 1.f }, { -1,  0,  0, 0 }, {  0,  0,  1, 0 }, { 0, 1, 0, 0 } },
			{ { -0.5, -0.5,  0.5, 1.f }, { -1,  0,  0, 0 }, {  0,  0,  1, 0 }, { 1, 1, 0, 0 } },
			{ { -0.5,  0.5,  0.5, 1.f }, { -1,  0,  0, 0 }, {  0,  0,  1, 0 }, { 1, 0, 0, 0 } },
			{ {  0.5,  0.5,  0.5, 1.f }, { -1,  0,  0, 0 }, {  0,  0,  1, 0 }, { 0, 0, 0, 0 } },
			// Left face
			{ { -0.5, -0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  0, -1, 0 }, { 0, 1, 0, 0 } },
			{ {  0.5, -0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  0, -1, 0 }, { 1, 1, 0, 0 } },
			{ {  0.5,  0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  0, -1, 0 }, { 1, 0, 0, 0 } },
			{ { -0.5,  0.5, -0.5, 1.f }, {  1,  0,  0, 0 }, {  0,  0, -1, 0 }, { 0, 0, 0, 0 } }
		};
		DirectX::XMINT4 indices[12]{
			{  0,  2,  1, 0 }, {  0,  3,  2, 0 },
			{  4,  6,  5, 0 }, {  4,  7,  6, 0 },
			{  8, 10,  9, 0 }, {  8, 11, 10, 0 },
			{ 12, 14, 13, 0 }, { 12, 15, 14, 0 },
			{ 16, 18, 17, 0 }, { 16, 19, 18, 0 },
			{ 20, 22, 21, 0 }, { 20, 23, 22, 0 }
		};
	} m_viBuffer{};

	struct CullParams {
		DirectX::XMINT4 shapeCount{}; // x - shapes count
		DirectX::SimpleMath::Vector4 bbMin[MaxInstances]{};
		DirectX::SimpleMath::Vector4 bbMax[MaxInstances]{};
	};

private:
	ID3D11Device* m_pDevice{};
	ID3D11DeviceContext* m_pDeviceContext{};

	ID3D11Buffer* m_pVertexBuffer{};
	ID3D11Buffer* m_pIndexBuffer{};

	ID3D11Buffer* m_pModelBufferInst{};
	ID3D11Buffer* m_pModelBufferInstVis{};
	std::vector<ModelBuffer> m_modelBuffers{ MaxInstances };
	std::vector<AABB> m_modelBBs{ MaxInstances };

	ID3D11VertexShader* m_pVertexShader{};
	ID3D11PixelShader* m_pPixelShader{};
	ID3D11InputLayout* m_pInputLayout{};

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

	// rt
	struct ModelBufferInv {
		DirectX::SimpleMath::Matrix modelMatrixInv{};
	};
	std::vector<ModelBufferInv> m_modelBuffersInv{ MaxInstances };
	ID3D11Buffer* m_pModelBufferInv{};

	ID3D11UnorderedAccessView* m_pRTTexture{};
	//VIBuffer m_viBuffer{};
	ID3D11Buffer* m_pVIBuffer{};
	ID3D11ComputeShader* m_pRTShader{};
	bool m_isRayTracing{ true };

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
		m_pDeviceContext(deviceContext) {}

	GPUTimer* m_pGPUTimer{};
	CPUTimer* m_pCPUTimer{};

	float getModelRotationSpeed();
	bool getIsDoCull();
	bool getIsRayTracing();
	int getInstCount();

	HRESULT init(DirectX::SimpleMath::Matrix* positions, int num);
	void term();

	void update(float delta, bool isRotate);
	void render(ID3D11SamplerState* sampler, ID3D11Buffer* SceneBuffer);

	HRESULT initCull();
	void calcFrustum(Camera& camera, float aspectRatio, DirectX::SimpleMath::Plane frustum[6]);
	void updateCullParams();
	void cullBoxes(ID3D11Buffer* m_pSceneBuffer, Camera& camera, float aspectRatio);
	void readQueries();

	void initImGUI();

	void rayTracingInit(ID3D11Texture2D* texture);
	void rayTracingUpdate(ID3D11Texture2D* texture);
	void rayTracing(ID3D11SamplerState* pSampler, ID3D11Buffer* m_pSceneBuffer, ID3D11Buffer* m_pRTBuffer, int width, int height);

private:
	HRESULT createVertexBuffer(Vertex(&vertices)[], UINT numVertices);
	HRESULT createIndexBuffer(USHORT(&indices)[], UINT numIndices);
	HRESULT createModelBuffer();
	HRESULT createTexture(TextureDesc& textureDesc, ID3D11Texture2D** texture);
	HRESULT createResourceView(TextureDesc& textureDesc, ID3D11Texture2D* pTexture, ID3D11ShaderResourceView** pShaderResourceView);

	void initModel(ModelBuffer& modelBuffer, AABB& bb);

};
