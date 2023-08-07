#include "Cube.h"

#include "ShaderProcessor.h"

ID3D11Buffer* Cube::getVertexBuffer() {
	return m_pVertexBuffer;
};

ID3D11Buffer* Cube::getIndexBuffer() {
	return m_pIndexBuffer;
};

ID3D11Buffer* Cube::getModelBuffer() {
	return m_pModelBuffer;
};

ID3D11VertexShader* Cube::getVertexShader() {
	return m_pVertexShader;
};

ID3D11PixelShader* Cube::getPixelShader() {
	return m_pPixelShader;
};

ID3D11InputLayout* Cube::getInputLayout() {
	return m_pInputLayout;
};

ID3D11Texture2D* Cube::getTexture() {
	return m_pTexture;
};

ID3D11ShaderResourceView* Cube::getTextureView() {
	return m_pTextureView;
};

HRESULT Cube::init() {
	HRESULT hr{ S_OK };

	// create vertex buffer
	{
		Vertex vertices[24]{
			// Bottom face
			{ { -0.5, -0.5,  0.5 }, { 0, 1 } },
			{ {  0.5, -0.5,  0.5 }, { 1, 1 } },
			{ {  0.5, -0.5, -0.5 }, { 1, 0 } },
			{ { -0.5, -0.5, -0.5 }, { 0, 0 } },
			// Front face
			{ { -0.5,  0.5, -0.5 }, { 0, 1 } },
			{ {  0.5,  0.5, -0.5 }, { 1, 1 } },
			{ {  0.5,  0.5,  0.5 }, { 1, 0 } },
			{ { -0.5,  0.5,  0.5 }, { 0, 0 } },
			// Top face
			{ {  0.5, -0.5, -0.5 }, { 0, 1 } },
			{ {  0.5, -0.5,  0.5 }, { 1, 1 } },
			{ {  0.5,  0.5,  0.5 }, { 1, 0 } },
			{ {  0.5,  0.5, -0.5 }, { 0, 0 } },
			// Back face
			{ { -0.5, -0.5,  0.5 }, { 0, 1 } },
			{ { -0.5, -0.5, -0.5 }, { 1, 1 } },
			{ { -0.5,  0.5, -0.5 }, { 1, 0 } },
			{ { -0.5,  0.5,  0.5 }, { 0, 0 } },
			// Right face
			{ {  0.5, -0.5,  0.5 }, { 0, 1 } },
			{ { -0.5, -0.5,  0.5 }, { 1, 1 } },
			{ { -0.5,  0.5,  0.5 }, { 1, 0 } },
			{ {  0.5,  0.5,  0.5 }, { 0, 0 } },
			// Left face
			{ { -0.5, -0.5, -0.5 }, { 0, 1 } },
			{ {  0.5, -0.5, -0.5 }, { 1, 1 } },
			{ {  0.5,  0.5, -0.5 }, { 1, 0 } },
			{ { -0.5,  0.5, -0.5 }, { 0, 0 } }
		};

		hr = createVertexBuffer(vertices, sizeof(vertices) / sizeof(*vertices));
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pVertexBuffer, "VertexBuffer");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create index buffer
	{
		UINT16 indices[36]{
			 0,	 2,  1,  0,  3,  2,
			 4,	 6,  5,  4,  7,  6,
			 8,	10,  9,  8, 11, 10,
			12, 14, 13, 12, 15, 14,
			16, 18, 17, 16, 19, 18,
			20, 22, 21, 20, 23, 22
		};

		hr = createIndexBuffer(indices, sizeof(indices) / sizeof(*indices));
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pIndexBuffer, "IndexBuffer");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// shaders processing
	ID3DBlob* vertexShaderCode{};
	{
		hr = compileAndCreateShader(
			m_pDevice,
			L"CubeShader.vs",
			(ID3D11DeviceChild**)&m_pVertexShader,
			&vertexShaderCode
		);
		if (FAILED(hr)) {
			return hr;
		}

		hr = compileAndCreateShader(
			m_pDevice,
			L"CubeShader.ps",
			(ID3D11DeviceChild**)&m_pPixelShader
		);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create input layout
	{
		D3D11_INPUT_ELEMENT_DESC InputDesc[]{
			{ 
				"POSITION",
				0,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				0,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			},
			{
				"TEXCOORD",
				0,
				DXGI_FORMAT_R32G32_FLOAT,
				0,
				12,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			}
		};

		hr = m_pDevice->CreateInputLayout(
			InputDesc,
			2,
			vertexShaderCode->GetBufferPointer(),
			vertexShaderCode->GetBufferSize(),
			&m_pInputLayout
		);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pInputLayout, "InputLayout");
		if (FAILED(hr)) {
			return hr;
		}
	}
	SAFE_RELEASE(vertexShaderCode);

	// create model buffer
	{
		CubeModelBuffer modelBuffer{ DirectX::XMMatrixIdentity() };

		hr = createModelBuffer(modelBuffer);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pModelBuffer, "GeomBuffer");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// load texture
	TextureDesc textureDesc{};
	{
		const std::wstring textureName{ L"../Common/Kitty.dds" };

		bool ddsRes{ LoadDDS(textureName.c_str(), textureDesc) };

		hr = createTexture(textureDesc);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pTexture, WCSToMBS(textureName));
		if (FAILED(hr)) {
			return hr;
		}
	}

	// create view
	{
		hr = createResourceView(textureDesc);
		if (FAILED(hr)) {
			return hr;
		}
	}
	free(textureDesc.pData);

	return hr;
}

HRESULT Cube::createVertexBuffer(Vertex(&vertices)[], UINT numVertices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(Vertex)* numVertices },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_VERTEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ vertices },
		.SysMemPitch{ sizeof(Vertex)* numVertices }
	};

	return m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
}

HRESULT Cube::createIndexBuffer(USHORT(&indices)[], UINT numIndices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(USHORT)* numIndices },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_INDEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ indices },
		.SysMemPitch{ sizeof(USHORT)* numIndices }
	};

	return m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
}

HRESULT Cube::createModelBuffer(CubeModelBuffer& modelBuffer) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(CubeModelBuffer) },
		.Usage{ D3D11_USAGE_DEFAULT },				// храним в VRAM, изменяем
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }	// константный
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ &modelBuffer },
		.SysMemPitch{ sizeof(modelBuffer) }
	};

	return m_pDevice->CreateBuffer(&desc, &data, &this->m_pModelBuffer);
}

HRESULT Cube::createTexture(TextureDesc& textureDesc) {
	D3D11_TEXTURE2D_DESC desc{
		// размеры в текселях
		textureDesc.width, textureDesc.height,
		textureDesc.mipmapsCount, 1,
		textureDesc.fmt, { 1, 0 },
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_SHADER_RESOURCE
	};

	// размеры текстуры в блоках
	UINT32 blockWidth{
		static_cast<UINT32>(std::ceil(desc.Width / 4))
	};
	UINT32 blockHeight{
		static_cast<UINT32>(std::ceil(desc.Height / 4))
	};

	// размер строки пикселей в байтах
	UINT32 pitch{
		blockWidth * GetBytesPerBlock(desc.Format)
	};
	const char* src{
		reinterpret_cast<const char*>(textureDesc.pData)
	};

	// расчет mip уровней
	std::vector<D3D11_SUBRESOURCE_DATA> data;
	data.resize(desc.MipLevels);
	for (UINT32 i{}; i < desc.MipLevels; ++i) {
		data[i].pSysMem = src;
		data[i].SysMemPitch = pitch;
		data[i].SysMemSlicePitch = 0;

		src += pitch * blockHeight;
		blockHeight = (std::max)(1u, blockHeight / 2);
		blockWidth = (std::max)(1u, blockWidth / 2);
		pitch = blockWidth * GetBytesPerBlock(desc.Format);
	}

	// создание текстуры
	return m_pDevice->CreateTexture2D(&desc, data.data(), &m_pTexture);
}

HRESULT Cube::createResourceView(TextureDesc& textureDesc) {
	// resource view для обработки в шейдерах
	D3D11_SHADER_RESOURCE_VIEW_DESC desc{
		.Format{ textureDesc.fmt },
		.ViewDimension{ D3D11_SRV_DIMENSION_TEXTURE2D },
		.Texture2D{
			.MostDetailedMip{},
			.MipLevels{ textureDesc.mipmapsCount }
		}
	};

	return m_pDevice->CreateShaderResourceView(m_pTexture, &desc, &m_pTextureView);
}

void Cube::term() {
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pModelBuffer);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pTextureView);
}

void Cube::update(float delta) {
	m_angleRotationY += m_modelRotationSpeed * delta;

	ModelBuffer modelBuffer{
		// матрица вращения вокруг оси
		DirectX::XMMatrixRotationAxis(
			// вектор, описывающий ось вращения
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
			m_angleRotationY
		)
	};

	// обновление буфера
	m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &modelBuffer, 0, 0);
}

void Cube::render(ID3D11SamplerState* sampler, ID3D11Buffer* viewProjectionBuffer) {
	ID3D11SamplerState* samplers[]{ sampler };
	m_pDeviceContext->PSSetSamplers(0, 1, samplers);

	ID3D11ShaderResourceView* resources[]{ m_pTextureView };
	m_pDeviceContext->PSSetShaderResources(0, 1, resources);

	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	ID3D11Buffer* vertexBuffers[]{ m_pVertexBuffer };
	UINT strides[]{ 20 };
	UINT offsets[]{ 0 };
	ID3D11Buffer* cbuffers[]{ viewProjectionBuffer, m_pModelBuffer };
	m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
	m_pDeviceContext->DrawIndexed(36, 0, 0);
}

float Cube::getModelRotationSpeed() {
	return m_modelRotationSpeed;
}
