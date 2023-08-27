#pragma once

#include "framework.h"

class LightSphere {
	typedef struct ModelBuffer {
		DirectX::SimpleMath::Matrix matrix;
		DirectX::SimpleMath::Color color;
	};

	ID3D11Device* m_pDevice{};
	ID3D11DeviceContext* m_pDeviceContext{};

	UINT m_indexCount{};
	ID3D11Buffer* m_pVertexBuffer{};
	ID3D11Buffer* m_pIndexBuffer{};
	ID3D11Buffer* m_pModelBuffers[10]{};
	ID3D11VertexShader* m_pVertexShader{};
	ID3D11PixelShader* m_pPixelShader{};
	ID3D11InputLayout* m_pInputLayout{};

public:
typedef struct Light {
	DirectX::SimpleMath::Vector4 pos{};
	DirectX::SimpleMath::Color color{
		1.0f, 1.0f, 1.0f, 0.0f
	};
} Light;

	LightSphere() = delete;
	LightSphere(ID3D11Device* device, ID3D11DeviceContext* deviceContext):
		m_pDevice(device),
		m_pDeviceContext(deviceContext)
	{}

	HRESULT init();
	void term();

	void update(
		Light (&lights)[10],
		int count
	);
	void render(
		ID3D11Buffer* viewProjectionBuffer,
		ID3D11DepthStencilState* m_pDepthState,
		ID3D11BlendState* m_pOpaqueBlendState,
		int count
	);

private:
	HRESULT createVertexBuffer(std::vector<DirectX::SimpleMath::Vector3>& vertices);
	HRESULT createIndexBuffer(std::vector<UINT16>& indices);
	HRESULT createModelBuffer();
};