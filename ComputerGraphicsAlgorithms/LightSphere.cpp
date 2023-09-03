#include "LightSphere.h"

#include "ShaderProcessor.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace {
	void getSphereDataSize(size_t latCells, size_t lonCells, size_t& vertexCount, size_t& indexCount) {
		vertexCount = (latCells + 1) * (lonCells + 1);
		indexCount = latCells * lonCells * 6;
	}
	
	void initSphereVertex(Vector3* vertices, size_t lat, size_t latCells, size_t lon, size_t lonCells) {
		float latAngle{ XM_PI * lat / latCells - XM_PIDIV2 };
		float lonAngle{ XM_2PI * lon / lonCells + XM_PI };

		Vector3 r{
			sinf(lonAngle) * cosf(latAngle),
			sinf(latAngle),
			cosf(lonAngle) * cosf(latAngle),
		};

		vertices[static_cast<int>(lat * (lonCells + 1) + lon)] = r / 2;
	}
	
	void initSphereVertexIndices(UINT16* indices, size_t lat, size_t latCells, size_t lon, size_t lonCells) {
		size_t index{ 6 * (lat * lonCells + lon) };

		indices[index + 0] = static_cast<UINT16>(lat * (latCells + 1) + lon + 0);
		indices[index + 2] = static_cast<UINT16>(lat * (latCells + 1) + lon + 1);
		indices[index + 1] = static_cast<UINT16>(lat * (latCells + 1) + lon + 1 + latCells);

		indices[index + 3] = static_cast<UINT16>(lat * (latCells + 1) + lon + 1);
		indices[index + 5] = static_cast<UINT16>(lat * (latCells + 1) + lon + 1 + latCells + 1);
		indices[index + 4] = static_cast<UINT16>(lat * (latCells + 1) + lon + 1 + latCells);
	}
	
	void createSphere(size_t latCells, size_t lonCells, Vector3* vertices, UINT16* indices) {
		for (size_t lat{}; lat < latCells + 1; ++lat) {
			for (size_t lon{}; lon < lonCells + 1; ++lon) {
				initSphereVertex(vertices, lat, latCells, lon, lonCells);
				if (lat != latCells && lon != lonCells) {
					initSphereVertexIndices(indices, lat, latCells, lon, lonCells);
				}
			}
		}
	}
}

HRESULT LightSphere::init() {
	HRESULT hr{ S_OK };

	size_t sphereSteps{ 8 };

	std::vector<Vector3> vertices;
	std::vector<UINT16> indices;

	size_t vertexCount;
	size_t indexCount;

	getSphereDataSize(sphereSteps, sphereSteps, vertexCount, indexCount);

	vertices.resize(vertexCount);
	indices.resize(indexCount);

	m_indexCount = static_cast<UINT>(indexCount);

	createSphere(sphereSteps, sphereSteps, vertices.data(), indices.data());

	for (auto& vertex : vertices) {
		vertex /= 8;
	}

	// create vertex buffer
	{
		hr = createVertexBuffer(vertices);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pVertexBuffer, "SmallSphereVertexBuffer");
		ThrowIfFailed(hr);
	}

	// create index buffer
	{
		hr = createIndexBuffer(indices);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pIndexBuffer, "SmallSphereIndexBuffer");
		ThrowIfFailed(hr);
	}

	// shaders processing
	ID3DBlob* pVertexShaderCode{};
	{
		hr = compileAndCreateShader(
			m_pDevice,
			L"TransColor.vs",
			reinterpret_cast<ID3D11DeviceChild**>(&m_pVertexShader),
			{},
			&pVertexShaderCode
		);
		ThrowIfFailed(hr);

		hr = compileAndCreateShader(
			m_pDevice,
			L"TransColor.ps",
			reinterpret_cast<ID3D11DeviceChild**>(&m_pPixelShader)
		);
	}

	// create input layout
	{
		D3D11_INPUT_ELEMENT_DESC InputDesc[]{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		hr = m_pDevice->CreateInputLayout(
			InputDesc,
			1,
			pVertexShaderCode->GetBufferPointer(),
			pVertexShaderCode->GetBufferSize(),
			&m_pInputLayout
		);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pInputLayout, "SmallSphereInputLayout");
		ThrowIfFailed(hr);
	}
	SAFE_RELEASE(pVertexShaderCode);

	// create model buffer
	{
		hr = createModelBuffer();
		ThrowIfFailed(hr);

		for (int i{}; i < 10 && SUCCEEDED(hr); ++i) {
			hr = SetResourceName(m_pModelBuffers[i], "SmallSphereModelBuffer" + i);
		}
		ThrowIfFailed(hr);
	}

	return hr;
}

HRESULT LightSphere::createVertexBuffer(std::vector<Vector3>& vertices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ static_cast<UINT>(sizeof(Vector3) * vertices.size()) },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_VERTEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ vertices.data() },
		.SysMemPitch{ static_cast<UINT>(sizeof(Vector3) * vertices.size()) }
	};

	return m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
}

HRESULT LightSphere::createIndexBuffer(std::vector<UINT16>& indices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ static_cast<UINT>(sizeof(UINT16) * indices.size()) },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_INDEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ indices.data() },
		.SysMemPitch{ static_cast<UINT>(sizeof(UINT16) * indices.size()) }
	};

	return m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
}

HRESULT LightSphere::createModelBuffer() {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(ModelBuffer) },
		.Usage{ D3D11_USAGE_DEFAULT },
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
	};

	ModelBuffer modelBuffer{
		Matrix::Identity,
		Vector4::One
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ &modelBuffer },
		.SysMemPitch{ sizeof(modelBuffer) }
	};

	HRESULT hr{ S_OK };
	for (int i{}; i < 10 && SUCCEEDED(hr); ++i) {
		m_pDevice->CreateBuffer(&desc, &data, &m_pModelBuffers[i]);
	}

	return hr;
}

void LightSphere::term() {
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	for (int i{}; i < 10; ++i) {
		SAFE_RELEASE(m_pModelBuffers[i]);
	}

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);
}

void LightSphere::update(Light (&lights)[10], int count) {
	for (int i{}; i < count; ++i) {
		ModelBuffer modelBuffer{
			XMMatrixTranslation(
				lights[i].pos.x,
				lights[i].pos.y,
				lights[i].pos.z
			),
			lights[i].color
		};

		m_pDeviceContext->UpdateSubresource(m_pModelBuffers[i], 0, nullptr, &modelBuffer, 0, 0);
	}
}

void LightSphere::render(
	ID3D11Buffer* viewProjectionBuffer,
	ID3D11DepthStencilState* m_pDepthState,
	ID3D11BlendState* m_pOpaqueBlendState,
	int count
) {
	m_pDeviceContext->OMSetBlendState(m_pOpaqueBlendState, nullptr, 0xffffffff);
	//m_pDeviceContext->OMSetDepthStencilState(m_pDepthState, 0);

	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
	UINT strides[] = { 12 };
	UINT offsets[] = { 0 };
	m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

	for (int i = 0; i < count; i++) {
		ID3D11Buffer* cbuffers[] = { viewProjectionBuffer, m_pModelBuffers[i] };
		m_pDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
		m_pDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
		m_pDeviceContext->DrawIndexed(m_indexCount, 0, 0);
	}
}

