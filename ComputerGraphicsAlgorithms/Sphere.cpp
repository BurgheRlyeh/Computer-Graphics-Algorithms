#include "Sphere.h"

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
		size_t index = 6 * (lat * lonCells + lon);

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

HRESULT Sphere::init() {
	HRESULT hr{ S_OK };

	size_t sphereSteps{ 32 };

	std::vector<Vector3> vertices;
	std::vector<UINT16> indices;

	size_t vertexCount;
	size_t indexCount;

	getSphereDataSize(sphereSteps, sphereSteps, vertexCount, indexCount);

	vertices.resize(vertexCount);
	indices.resize(indexCount);

	m_indexCount = static_cast<UINT>(indexCount);

	createSphere(sphereSteps, sphereSteps, vertices.data(), indices.data());

	// create vertex buffer
	{
		hr = createVertexBuffer(vertices);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pVertexBuffer, "SphereVertexBuffer");
		ThrowIfFailed(hr);
	}

	// create index buffer
	{
		hr = createIndexBuffer(indices);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pIndexBuffer, "SphereIndexBuffer");
		ThrowIfFailed(hr);
	}

	// shaders processing
	ID3DBlob* sphereVertexShaderCode{};
	{
		hr = compileAndCreateShader(
			m_pDevice,
			L"SphereShader.vs",
			reinterpret_cast<ID3D11DeviceChild**>(&m_pVertexShader),
			{},
			&sphereVertexShaderCode
		);
		ThrowIfFailed(hr);

		hr = compileAndCreateShader(
			m_pDevice,
			L"SphereShader.ps",
			reinterpret_cast<ID3D11DeviceChild**>(&m_pPixelShader)
		);
		ThrowIfFailed(hr);
	}

	// create input layout
	{
		D3D11_INPUT_ELEMENT_DESC InputDesc[]{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		hr = m_pDevice->CreateInputLayout(
			InputDesc,
			1,
			sphereVertexShaderCode->GetBufferPointer(),
			sphereVertexShaderCode->GetBufferSize(),
			&m_pInputLayout
		);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pInputLayout, "SphereInputLayout");
		ThrowIfFailed(hr);
	}
	SAFE_RELEASE(sphereVertexShaderCode);

	// create geometry buffer
	{
		hr = createModelBuffer();
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pModelBuffer, "SphereModelBuffer");
		ThrowIfFailed(hr);
	}

	// create texture
	TextureDesc texDescs[6];
	{
		std::wstring TextureNames[6]{
			L"../Common/posx.dds", L"../Common/negx.dds",
			L"../Common/posy.dds", L"../Common/negy.dds",
			L"../Common/posz.dds", L"../Common/negz.dds"
		};
		bool ddsRes{ true };
		for (int i{}; i < 6 && ddsRes; ++i) {
			ddsRes = LoadDDS(TextureNames[i].c_str(), texDescs[i], true);
		}

		hr = createTexture(texDescs);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pCubeMapTexture, "CubemapTexture");
		ThrowIfFailed(hr);
	}

	// create resource view
	{
		hr = createResourceView(texDescs);
		ThrowIfFailed(hr);
	}
	for (int i = 0; i < 6; i++) {
		free(texDescs[i].pData);
	}

	return hr;
}

HRESULT Sphere::createVertexBuffer(std::vector<Vector3>& vertices) {
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

HRESULT Sphere::createIndexBuffer(std::vector<UINT16>& indices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ static_cast<UINT>(sizeof(UINT16) * indices.size()) },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_INDEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ indices.data() },
		.SysMemPitch{ static_cast<UINT>(sizeof(UINT16)* indices.size()) }
	};

	return m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
}

HRESULT Sphere::createModelBuffer() {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(ModelBuffer) },
		.Usage{ D3D11_USAGE_DEFAULT },
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
	};

	ModelBuffer modelBuffer{
		.matrix{ Matrix::Identity },
		.size{ 2.0f, 0.0f, 0.0f, 0.0f }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ &modelBuffer },
		.SysMemPitch{ sizeof(modelBuffer) }
	};

	return m_pDevice->CreateBuffer(&desc, &data, &m_pModelBuffer);
}

HRESULT Sphere::createTexture(TextureDesc(&texDescs)[]) {
	D3D11_TEXTURE2D_DESC desc{
			.Width{ texDescs[0].height },
			.Height{ texDescs[0].width },
			.MipLevels{ 1 },
			.ArraySize{ 6 },
			.Format{ texDescs[0].fmt },
			.SampleDesc{ 1, 0 },
			.Usage{ D3D11_USAGE_IMMUTABLE },
			.BindFlags{ D3D11_BIND_SHADER_RESOURCE },
			.CPUAccessFlags{},
			.MiscFlags{ D3D11_RESOURCE_MISC_TEXTURECUBE }
	};

	UINT32 blockWidth{ DivUp(desc.Width, 4u) };
	UINT32 blockHeight{ DivUp(desc.Height, 4u) };
	UINT32 pitch{ blockWidth * GetBytesPerBlock(desc.Format) };

	D3D11_SUBRESOURCE_DATA data[6];
	for (int i{}; i < 6; ++i) {
		data[i].pSysMem = texDescs[i].pData;
		data[i].SysMemPitch = pitch;
		data[i].SysMemSlicePitch = 0;
	}

	return m_pDevice->CreateTexture2D(&desc, data, &m_pCubeMapTexture);
}

HRESULT Sphere::createResourceView(TextureDesc(&texDescs)[]) {
	D3D11_SHADER_RESOURCE_VIEW_DESC desc{
		.Format{ texDescs[0].fmt },
		.ViewDimension{ D3D_SRV_DIMENSION_TEXTURECUBE },
		.TextureCube{
			.MostDetailedMip{},
			.MipLevels{ 1 }
		}
	};

	return m_pDevice->CreateShaderResourceView(m_pCubeMapTexture, &desc, &m_pCubeMapView);
}

void Sphere::term() {
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pModelBuffer);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pCubeMapTexture);
	SAFE_RELEASE(m_pCubeMapView);
}

void Sphere::resize(float r) {
	ModelBuffer modelBuffer{
		.matrix{ Matrix::Identity },
		.size{ r, 0.0f, 0.0f, 0.0f }
	};
	m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &modelBuffer, 0, 0);
}

void Sphere::render(ID3D11SamplerState* sampler, ID3D11Buffer* viewProjectionBuffer) {
	ID3D11SamplerState* samplers[]{ sampler };
	m_pDeviceContext->PSSetSamplers(0, 1, samplers);

	ID3D11ShaderResourceView* resources[]{ m_pCubeMapView };
	m_pDeviceContext->PSSetShaderResources(0, 1, resources);

	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	ID3D11Buffer* vertexBuffers[]{ m_pVertexBuffer };
	UINT strides[]{ 12 };
	UINT offsets[]{ 0 };
	ID3D11Buffer* cbuffers[]{ viewProjectionBuffer, m_pModelBuffer };
	m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
	m_pDeviceContext->DrawIndexed(m_indexCount, 0, 0);
}
