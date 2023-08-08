#pragma once

#include "framework.h"
#include "Texture.h"

class Rect {
	typedef struct Vertex {
		DirectX::XMFLOAT3 point;
		COLORREF color;
	} Vertex;

	typedef struct ModelBuffer {
		DirectX::XMMATRIX matrix;
		DirectX::XMFLOAT4 color;
	};

	typedef struct BoundingRect {
		DirectX::XMFLOAT3 v[4];
	} BoundingRect;

	ID3D11Device* m_pDevice{};
	ID3D11DeviceContext* m_pDeviceContext{};

	ID3D11Buffer* m_pVertexBuffer{};
	ID3D11Buffer* m_pIndexBuffer{};

	std::vector<ID3D11Buffer*> m_pModelBuffers{};
	std::vector<BoundingRect> m_boundingRects{};

	ID3D11VertexShader* m_pVertexShader{};
	ID3D11PixelShader* m_pPixelShader{};
	ID3D11InputLayout* m_pInputLayout{};

public:
	Rect(ID3D11Device* device, ID3D11DeviceContext* deviceContext):
		m_pDevice(device),
		m_pDeviceContext(deviceContext) {}

	HRESULT init(DirectX::XMFLOAT3* positions, DirectX::XMFLOAT4* colors, int num);
	void term();

	void update(DirectX::XMMATRIX matrix);
	void render(
		ID3D11SamplerState* sampler,
		ID3D11Buffer* viewProjectionBuffer,
		ID3D11DepthStencilState* m_pTransDepthState,
		ID3D11BlendState* m_pTransBlendState,
		DirectX::XMFLOAT3 cameraPos
	);

private:
	HRESULT createVertexBuffer(Vertex(&vertices)[], UINT numVertices);
	HRESULT createIndexBuffer(USHORT(&indices)[], UINT numIndices);
	HRESULT createModelBuffer(ModelBuffer& modelBuffer, int idx);
}; 