#pragma once

#define NOMINMAX
#include <limits>

#include "framework.h"
#include "Texture.h"
#include "Camera.h"
#include "AABB.h"
#include "Timer.h"
#include "BVH.h"

struct TextureDesc;
class Renderer;
struct Camera;
struct AABB;
class CPUTimer;
class GPUTimer;
class BVH;

#define LIMIT_V 1013
#define LIMIT_I 1107

class Geometry {

public:
	struct ModelBuffer {
		DirectX::SimpleMath::Matrix mModel{};
		DirectX::SimpleMath::Matrix mModelInv{};
		DirectX::SimpleMath::Matrix mNormal{};
		DirectX::SimpleMath::Vector4 shineSpeedTexIdNM{};
		DirectX::SimpleMath::Vector4 posAngle{};

		void updateMatrices();
	};

	struct VIBuffer {
		DirectX::SimpleMath::Vector4 vertices[LIMIT_V]{};
		DirectX::XMINT4 indices[LIMIT_I]{};
	};

	BVH bvh{};

private:
	ID3D11Device* m_pDevice{};
	ID3D11DeviceContext* m_pDeviceContext{};

	// model buffer
	ModelBuffer m_modelBuffer{};
	ID3D11Buffer* m_pModelBuffer{};

	// vertex-index buffer
	VIBuffer m_viBuffer{};
	ID3D11Buffer* m_pVIBuffer{};

	float m_modelRotationSpeed{ DirectX::XM_PIDIV2 };

	// rt
	ID3D11Buffer* m_pAABBBuffer{};
	ID3D11Buffer* m_pLFCPBuffer{};
	ID3D11Buffer* m_pTriBuffer{};

	ID3D11UnorderedAccessView* m_pUAVTexture{};

	ID3D11ComputeShader* m_pRayTracingCS{};

	bool m_isRayTracing{ true };
	bool m_isRebuildBVH{ false };

public:
	Geometry() = delete;
	Geometry(ID3D11Device* device, ID3D11DeviceContext* deviceContext) :
		m_pDevice(device),
		m_pDeviceContext(deviceContext) {}

	GPUTimer* m_pGPUTimer{};
	CPUTimer* m_pCPUTimer{};

	float getModelRotationSpeed();
	bool getIsRayTracing();

	HRESULT init(ID3D11Texture2D* tex);
	void term();

	void update(float delta, bool isRotate);
	void updateBVH(bool rebuild = false);

	void resizeUAV(ID3D11Texture2D* texture);
	void rayTracing(ID3D11Buffer* m_pSceneBuffer, ID3D11Buffer* m_pRTBuffer, int width, int height);
};
