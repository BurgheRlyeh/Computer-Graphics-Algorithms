#include "Cube.h"

#include "ShaderProcessor.h"

#include "../Common/imgui/imgui.h"
#include "../Common/imgui/backends/imgui_impl_dx11.h"
#include "../Common/imgui/backends/imgui_impl_win32.h"

HRESULT Cube::init(DirectX::XMMATRIX* positions, int num) {
	HRESULT hr{ S_OK };

	// create vertex buffer
	{
		Vertex vertices[24]{
			// Bottom face
			{ { -0.5, -0.5,  0.5 }, {  1,  0,  0 }, {  0, -1,  0 }, { 0, 1 } },
			{ {  0.5, -0.5,  0.5 }, {  1,  0,  0 }, {  0, -1,  0 }, { 1, 1 } },
			{ {  0.5, -0.5, -0.5 }, {  1,  0,  0 }, {  0, -1,  0 }, { 1, 0 } },
			{ { -0.5, -0.5, -0.5 }, {  1,  0,  0 }, {  0, -1,  0 }, { 0, 0 } },
			// Front face 
			{ { -0.5,  0.5, -0.5 }, {  1,  0,  0 }, {  0,  1,  0 }, { 0, 1 } },
			{ {  0.5,  0.5, -0.5 }, {  1,  0,  0 }, {  0,  1,  0 }, { 1, 1 } },
			{ {  0.5,  0.5,  0.5 }, {  1,  0,  0 }, {  0,  1,  0 }, { 1, 0 } },
			{ { -0.5,  0.5,  0.5 }, {  1,  0,  0 }, {  0,  1,  0 }, { 0, 0 } },
			// Top face
			{ {  0.5, -0.5, -0.5 }, {  0,  0,  1 }, {  1,  0,  0 }, { 0, 1 } },
			{ {  0.5, -0.5,  0.5 }, {  0,  0,  1 }, {  1,  0,  0 }, { 1, 1 } },
			{ {  0.5,  0.5,  0.5 }, {  0,  0,  1 }, {  1,  0,  0 }, { 1, 0 } },
			{ {  0.5,  0.5, -0.5 }, {  0,  0,  1 }, {  1,  0,  0 }, { 0, 0 } },
			// Back face
			{ { -0.5, -0.5,  0.5 }, {  0,  0, -1 }, { -1,  0,  0 }, { 0, 1 } },
			{ { -0.5, -0.5, -0.5 }, {  0,  0, -1 }, { -1,  0,  0 }, { 1, 1 } },
			{ { -0.5,  0.5, -0.5 }, {  0,  0, -1 }, { -1,  0,  0 }, { 1, 0 } },
			{ { -0.5,  0.5,  0.5 }, {  0,  0, -1 }, { -1,  0,  0 }, { 0, 0 } },
			// Right face
			{ {  0.5, -0.5,  0.5 }, { -1,  0,  0 }, {  0,  0,  1 }, { 0, 1 } },
			{ { -0.5, -0.5,  0.5 }, { -1,  0,  0 }, {  0,  0,  1 }, { 1, 1 } },
			{ { -0.5,  0.5,  0.5 }, { -1,  0,  0 }, {  0,  0,  1 }, { 1, 0 } },
			{ {  0.5,  0.5,  0.5 }, { -1,  0,  0 }, {  0,  0,  1 }, { 0, 0 } },
			// Left face
			{ { -0.5, -0.5, -0.5 }, {  1,  0,  0 }, {  0,  0, -1 }, { 0, 1 } },
			{ {  0.5, -0.5, -0.5 }, {  1,  0,  0 }, {  0,  0, -1 }, { 1, 1 } },
			{ {  0.5,  0.5, -0.5 }, {  1,  0,  0 }, {  0,  0, -1 }, { 1, 0 } },
			{ { -0.5,  0.5, -0.5 }, {  1,  0,  0 }, {  0,  0, -1 }, { 0, 0 } }
		};

		hr = createVertexBuffer(vertices, sizeof(vertices) / sizeof(*vertices));
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pVertexBuffer, "CubeVertexBuffer");
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

		hr = SetResourceName(m_pIndexBuffer, "CubeIndexBuffer");
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
			{},
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
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		hr = m_pDevice->CreateInputLayout(
			InputDesc,
			4,
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
		hr = createModelBuffer();
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pModelBufferInst, "ModelBufferInst");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// init models
	{
		const float diag = sqrtf(2.0f) / 2.0f * 0.5f;

		m_modelBuffers[0].settings.x = 0.0f;
		m_modelBuffers[0].settings.y = m_modelRotationSpeed;
		m_modelBuffers[0].settings.z = 0.0f;
		int useNM{ 1 };
		m_modelBuffers[0].settings.w = *reinterpret_cast<float*>(&useNM);
		m_modelBuffers[0].posAndAng = DirectX::XMFLOAT4{ 0.00001f, 0.0f, 0.0f, 0.0f };
		m_modelBBs[0].vmin = DirectX::XMFLOAT3{
			m_modelBuffers[0].posAndAng.x - diag,
			m_modelBuffers[0].posAndAng.y - 0.5f,
			m_modelBuffers[0].posAndAng.z - diag
		};
		m_modelBBs[0].vmax = DirectX::XMFLOAT3{
			m_modelBuffers[0].posAndAng.x + diag,
			m_modelBuffers[0].posAndAng.y + 0.5f,
			m_modelBuffers[0].posAndAng.z + diag
		};

		m_modelBuffers[1].settings.x = 64.0f;
		m_modelBuffers[1].settings.y = 0.0f;
		m_modelBuffers[1].settings.z = 0.0f;
		m_modelBuffers[1].settings.w = *reinterpret_cast<float*>(&useNM);
		m_modelBuffers[1].posAndAng = DirectX::XMFLOAT4{ 2.0f, 0.0f, 0.0f, 0.0f };
		DirectX::XMMATRIX m{
			DirectX::XMMatrixMultiply(
				DirectX::XMMatrixRotationAxis(
					DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
					-(float)m_modelBuffers[1].posAndAng.w
				),
				DirectX::XMMatrixTranslation(
					m_modelBuffers[1].posAndAng.x,
					m_modelBuffers[1].posAndAng.y,
					m_modelBuffers[1].posAndAng.z
				)
			)
		};
		m_modelBuffers[1].matrix = m;
		m_modelBuffers[1].normalMatrix = DirectX::XMMatrixTranspose(
			DirectX::XMMatrixInverse(nullptr, m)
		);
		m_modelBBs[1].vmin = DirectX::XMFLOAT3{
			m_modelBuffers[1].posAndAng.x - 0.5f,
			m_modelBuffers[1].posAndAng.y - 0.5f,
			m_modelBuffers[1].posAndAng.z - 0.5f
		};
		m_modelBBs[1].vmax = DirectX::XMFLOAT3{
			m_modelBuffers[1].posAndAng.x + 0.5f,
			m_modelBuffers[1].posAndAng.y + 0.5f,
			m_modelBuffers[1].posAndAng.z + 0.5f
		};

		for (int i{ 2 }; i < 10; ++i) {
			initModel(m_modelBuffers[i], m_modelBBs[i]);
		}
		m_instCount = 10;
	}

	// create model visibility buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(DirectX::XMINT4) * MaxInstances },
			.Usage{ D3D11_USAGE_DYNAMIC },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER },
			.CPUAccessFlags{ D3D11_CPU_ACCESS_WRITE }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pModelBufferInstVis);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pModelBufferInstVis, "ModelBufferInstVis");
		if (FAILED(hr)) {
			return hr;
		}
	}

	// load texture and create view 
	{
		TextureDesc textureDesc[2];

		const std::wstring textureName{ L"../Common/Brick.dds" };

		bool ddsRes{ LoadDDS(textureName.c_str(), textureDesc[0])};
		if (ddsRes) {
			ddsRes = LoadDDS(L"../Common/Kitty.dds", textureDesc[1]);
		}

		D3D11_TEXTURE2D_DESC desc{
			textureDesc[0].width, textureDesc[0].height,
			textureDesc[0].mipmapsCount, 2, // ArraySize = 2
			textureDesc[0].fmt, { 1, 0 },
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_SHADER_RESOURCE
		};

		std::vector<D3D11_SUBRESOURCE_DATA> data;
		data.resize(desc.MipLevels * 2);
		for (UINT32 j{}; j < 2; ++j) {
			// размеры текстуры в блоках
			UINT32 blockWidth{ static_cast<UINT32>(std::ceil(desc.Width / 4)) };
			UINT32 blockHeight{ static_cast<UINT32>(std::ceil(desc.Height / 4)) };

			// размер строки пикселей в байтах
			UINT32 pitch{ blockWidth * GetBytesPerBlock(desc.Format) };
			const char* src{ reinterpret_cast<const char*>(textureDesc[j].pData) };

			// расчет mip уровней
			for (UINT32 i{}; i < desc.MipLevels; ++i) {
				data[j * desc.MipLevels + i].pSysMem = src;
				data[j * desc.MipLevels + i].SysMemPitch = pitch;
				data[j * desc.MipLevels + i].SysMemSlicePitch = 0;

				src += pitch * blockHeight;
				blockHeight = (std::max)(1u, blockHeight / 2);
				blockWidth = (std::max)(1u, blockWidth / 2);
				pitch = blockWidth * GetBytesPerBlock(desc.Format);
			}
		}

		// создание текстуры
		m_pDevice->CreateTexture2D(&desc, data.data(), &m_pTexture);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pTexture, WCSToMBS(L"Diffuse textures"));
		if (FAILED(hr)) {
			return hr;
		}

		// resource view для обработки в шейдерах
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format{ textureDesc[0].fmt },
			.ViewDimension{ D3D11_SRV_DIMENSION_TEXTURE2DARRAY },
			.Texture2DArray{
				.MipLevels{ textureDesc[0].mipmapsCount },
				.ArraySize{ 2 },
			}
		};

		hr = m_pDevice->CreateShaderResourceView(m_pTexture, &srvDesc, &m_pTextureView);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pTexture, WCSToMBS(L"Diffuse texture views"));
		if (FAILED(hr)) {
			return hr;
		}

		for (UINT32 j{}; j < 2; ++j) {
			free(textureDesc[j].pData);
		}
	}

	// load texture and create view for normal map
	{
		TextureDesc textureDesc{};

		const std::wstring textureName{ L"../Common/BrickNM.dds" };

		bool ddsRes{ LoadDDS(textureName.c_str(), textureDesc) };

		hr = createTexture(textureDesc, &m_pTextureNM);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pTextureNM, WCSToMBS(textureName));
		if (FAILED(hr)) {
			return hr;
		}

		hr = createResourceView(textureDesc, m_pTextureNM, &m_pTextureViewNM);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SetResourceName(m_pTexture, "CubeTextureViewNM");
		if (FAILED(hr)) {
			return hr;
		}

		free(textureDesc.pData);
	}

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

HRESULT Cube::createModelBuffer() {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(ModelBuffer) * MaxInstances },
		.Usage{ D3D11_USAGE_DEFAULT },
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
	};

	return m_pDevice->CreateBuffer(&desc, nullptr, &m_pModelBufferInst);
}

HRESULT Cube::createTexture(TextureDesc& textureDesc, ID3D11Texture2D** texture) {
	D3D11_TEXTURE2D_DESC desc{
		// размеры в текселях
		textureDesc.width, textureDesc.height,
		textureDesc.mipmapsCount, 1,
		textureDesc.fmt, { 1, 0 },
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_SHADER_RESOURCE
	};

	// размеры текстуры в блоках
	UINT32 blockWidth{ static_cast<UINT32>(std::ceil(desc.Width / 4)) };
	UINT32 blockHeight{ static_cast<UINT32>(std::ceil(desc.Height / 4)) };

	// размер строки пикселей в байтах
	UINT32 pitch{ blockWidth * GetBytesPerBlock(desc.Format) };
	const char* src{ reinterpret_cast<const char*>(textureDesc.pData) };

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
	return m_pDevice->CreateTexture2D(&desc, data.data(), texture);
}

HRESULT Cube::createResourceView(TextureDesc& textureDesc, ID3D11Texture2D* pTexture, ID3D11ShaderResourceView** ppSRView) {
	// resource view для обработки в шейдерах
	D3D11_SHADER_RESOURCE_VIEW_DESC desc{
		.Format{ textureDesc.fmt },
		.ViewDimension{ D3D11_SRV_DIMENSION_TEXTURE2D },
		.Texture2D{
			.MostDetailedMip{},
			.MipLevels{ textureDesc.mipmapsCount }
		}
	};

	return m_pDevice->CreateShaderResourceView(pTexture, &desc, ppSRView);
}

void Cube::term() {
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pModelBufferInst);
	SAFE_RELEASE(m_pModelBufferInstVis);

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pTextureView);

	SAFE_RELEASE(m_pTextureNM);
	SAFE_RELEASE(m_pTextureViewNM);
}

void Cube::update(
	float delta,
	bool isRotate
) {
	if (isRotate) {
		for (UINT i{}; i < m_instCount; ++i) {
			if (fabs(m_modelBuffers[i].settings.y) > 0.0001) {
				m_modelBuffers[i].posAndAng.w += delta * m_modelBuffers[i].settings.y;
			}

			// model matrix
			// angle is reversed, as DirectXMatr calculates it as clockwise
			DirectX::XMMATRIX m{
				DirectX::XMMatrixMultiply(
					DirectX::XMMatrixRotationAxis(
						DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
						-m_modelBuffers[i].posAndAng.w
					),
					DirectX::XMMatrixTranslation(
						m_modelBuffers[i].posAndAng.x,
						m_modelBuffers[i].posAndAng.y,
						m_modelBuffers[i].posAndAng.z
					)
				)
			};

			m_modelBuffers[i].matrix = m;
			m_modelBuffers[i].normalMatrix = DirectX::XMMatrixTranspose(
				DirectX::XMMatrixInverse(nullptr, m)
			);
		}
	}

	m_pDeviceContext->UpdateSubresource(m_pModelBufferInst, 0, nullptr, m_modelBuffers.data(), 0, 0);
}

void Cube::render(ID3D11SamplerState* pSampler, ID3D11Buffer* pSceneBuffer) {
	ID3D11SamplerState* samplers[]{ pSampler };
	m_pDeviceContext->PSSetSamplers(0, 1, samplers);

	ID3D11ShaderResourceView* resources[]{ m_pTextureView, m_pTextureViewNM };
	m_pDeviceContext->PSSetShaderResources(0, 2, resources);

	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	ID3D11Buffer* vertexBuffers[]{ m_pVertexBuffer };
	UINT strides[]{ 44 };
	UINT offsets[]{ 0 };
	m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);

	ID3D11Buffer* cbuffers[]{ pSceneBuffer, m_pModelBufferInst, m_pModelBufferInstVis };
	m_pDeviceContext->VSSetConstantBuffers(0, 3, cbuffers);
	m_pDeviceContext->PSSetConstantBuffers(0, 3, cbuffers);

	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

	m_pDeviceContext->DrawIndexedInstanced(
		36,
		m_doCull ? m_instVisCount : m_instCount,
		0, 0, 0
	);
}

float Cube::getModelRotationSpeed() {
	return m_modelRotationSpeed;
}

void Cube::initImGUI() {
	ImGui::Begin("Instances");
	bool add = ImGui::Button("+");
	ImGui::SameLine();
	bool remove = ImGui::Button("-");
	ImGui::Text("Count %d", m_instCount);
	ImGui::Text("Visible %d", m_instVisCount);
	ImGui::Checkbox("Cull", &m_doCull);
	ImGui::SameLine();
	ImGui::End();
	if (add && m_instCount < MaxInstances) {
		DirectX::XMFLOAT4 pos{
			m_modelBuffers[m_instCount].posAndAng
		};
		if (!pos.x && !pos.y && !pos.z) {
			initModel(m_modelBuffers[m_instCount], m_modelBBs[m_instCount]);
		}
		++m_instCount;
	}
	if (remove && m_instCount) {
		--m_instCount;
	}
}
void Cube::initModel(ModelBuffer& modelBuffer, AABB& bb) {
	DirectX::XMFLOAT3 offset{
		7.0f * randNormf() - 3.5f,
		7.0f * randNormf() - 3.5f,
		7.0f * randNormf() - 3.5f
	};

	modelBuffer.settings.x = 0.5f < randNormf() ? 64.0f : 0.0f;
	modelBuffer.settings.y = DirectX::XM_2PI * randNormf();
	modelBuffer.posAndAng = { offset.x, offset.y, offset.z, 0.0f };

	const float diag = sqrtf(2.0f) / 2.0f * 0.5f;
	bb.vmin = {
		modelBuffer.posAndAng.x - diag,
		modelBuffer.posAndAng.y - 0.5f,
		modelBuffer.posAndAng.z - diag,
	};
	bb.vmax = {
		modelBuffer.posAndAng.x + diag,
		modelBuffer.posAndAng.y + 0.5f,
		modelBuffer.posAndAng.z + diag,
	};

	int useNM{ 1 };
	if (randNormf() > 0.5f) {
		modelBuffer.settings.z = 1.0f;
		useNM = 0;
	}
	else {
		modelBuffer.settings.z = 0.0f;
		useNM = 1;
	}

	modelBuffer.settings.w = *reinterpret_cast<float*>(&useNM);
}


// build plane equation on 4 points
DirectX::XMFLOAT4 BuildPlane(
	const DirectX::XMFLOAT3& p0,
	const DirectX::XMFLOAT3& p1,
	const DirectX::XMFLOAT3& p2,
	const DirectX::XMFLOAT3& p3
) {
	auto v0{ DirectX::XMLoadFloat3(&p0) };
	auto v1{ DirectX::XMLoadFloat3(&p1) };
	auto v2{ DirectX::XMLoadFloat3(&p2) };
	auto v3{ DirectX::XMLoadFloat3(&p3) };

	auto normVector{
		DirectX::XMVector3Cross(
			DirectX::XMVectorSubtract(v1, v0),
			DirectX::XMVectorSubtract(v3, v0)
		)
	};
	DirectX::XMFLOAT3 normPoint{};
	DirectX::XMStoreFloat3(&normPoint, normVector);

	float length{
		sqrtf(powf(normPoint.x, 2) + powf(normPoint.y, 2) + powf(normPoint.z, 2))
	};

	//DirectX::XMVector3Normalize(normVector);
	DirectX::XMFLOAT3 norm{
		normPoint.x / length,
		normPoint.y / length,
		normPoint.z / length
	};
	normVector = DirectX::XMLoadFloat3(&norm);
	//DirectX::XMStoreFloat3(&norm, normVector);

	auto posVector{ DirectX::XMVectorAdd(v0,
		DirectX::XMVectorAdd(v1,
			DirectX::XMVectorAdd(v2, v3)
		)
	) };
	DirectX::XMVectorScale(posVector, 0.25f);
	float w{ DirectX::XMVectorGetX(
		DirectX::XMVector3Dot(posVector, normVector)
	) };
	

	return {
		norm.x,
		norm.y,
		norm.z,
		- w / 4
	};
}

// is box inside
bool isBoxInside(
	const DirectX::XMFLOAT4 frustum[6],
	const DirectX::XMFLOAT3& bbMin,
	const DirectX::XMFLOAT3& bbMax
) {
	for (int i{}; i < 6; ++i) {
		const DirectX::XMFLOAT3 norm{
			frustum[i].x,
				frustum[i].y,
				frustum[i].z
		};
		DirectX::XMFLOAT4 p{
			signbit(norm.x) ? bbMin.x : bbMax.x,
				signbit(norm.y) ? bbMin.y : bbMax.y,
				signbit(norm.z) ? bbMin.z : bbMax.z,
				1.0f
		};
		float s{
			p.x * frustum[i].x +
			p.y * frustum[i].y +
			p.z * frustum[i].z +
			p.w * frustum[i].w
		};
		if (s < 0.0f) {
			return false;
		}
	}
	return true;
}

void Cube::cullBoxes(Camera& camera, float aspectRatio) {
	DirectX::XMFLOAT3 dir{ camera.getDir() };
	dir.x *= -1;
	dir.y *= -1;
	dir.z *= -1;

	DirectX::XMFLOAT3 up{ camera.getUp() };
	DirectX::XMFLOAT3 right{ camera.getRight() };
	DirectX::XMFLOAT3 pos{ camera.getPosition() };

	float n{ 0.1f };
	float f{ 100.0f };
	float fov{ DirectX::XM_PI / 3 };

	float x{ n * tanf(fov / 2) };
	float y{ n * tanf(fov / 2) * aspectRatio };

	DirectX::XMFLOAT3 nearVertices[4]{
		{
			pos.x + (dir.x * n) - (up.x * y) - (right.x * x),
			pos.y + (dir.y * n) - (up.y * y) - (right.y * x),
			pos.z + (dir.z * n) - (up.z * y) - (right.z * x)
		},
		{
			pos.x + (dir.x * n) - (up.x * y) + (right.x * x),
			pos.y + (dir.y * n) - (up.y * y) + (right.y * x),
			pos.z + (dir.z * n) - (up.z * y) + (right.z * x)
		},
		{
			pos.x + (dir.x * n) + (up.x * y) + (right.x * x),
			pos.y + (dir.y * n) + (up.y * y) + (right.y * x),
			pos.z + (dir.z * n) + (up.z * y) + (right.z * x)
		},
		{
			pos.x + (dir.x * n) + (up.x * y) - (right.x * x),
			pos.y + (dir.y * n) + (up.y * y) - (right.y * x),
			pos.z + (dir.z * n) + (up.z * y) - (right.z * x)
		}
	};

	x = f * tanf(fov / 2);
	y = f * tanf(fov / 2) * aspectRatio;

	DirectX::XMFLOAT3 farVertices[4]{
		{
			pos.x + (dir.x * f) - (up.x * y) - (right.x * x),
			pos.y + (dir.y * f) - (up.y * y) - (right.y * x),
			pos.z + (dir.z * f) - (up.z * y) - (right.z * x)
		},
		{
			pos.x + (dir.x * f) - (up.x * y) + (right.x * x),
			pos.y + (dir.y * f) - (up.y * y) + (right.y * x),
			pos.z + (dir.z * f) - (up.z * y) + (right.z * x)
		},
		{
			pos.x + (dir.x * f) + (up.x * y) + (right.x * x),
			pos.y + (dir.y * f) + (up.y * y) + (right.y * x),
			pos.z + (dir.z * f) + (up.z * y) + (right.z * x)
		},
		{
			pos.x + (dir.x * f) + (up.x * y) - (right.x * x),
			pos.y + (dir.y * f) + (up.y * y) - (right.y * x),
			pos.z + (dir.z * f) + (up.z * y) - (right.z * x)
		}
	};

	DirectX::XMFLOAT4 frustum[6]{
		BuildPlane(nearVertices[0], nearVertices[1], nearVertices[2], nearVertices[3]),
		BuildPlane(nearVertices[0], farVertices[0], farVertices[1], nearVertices[1]),
		BuildPlane(nearVertices[1], farVertices[1], farVertices[2], nearVertices[2]),
		BuildPlane(nearVertices[2], farVertices[2], farVertices[3], nearVertices[3]),
		BuildPlane(nearVertices[3], farVertices[3], farVertices[0], nearVertices[0]),
		BuildPlane(farVertices[1], farVertices[0], farVertices[3], farVertices[2])
	};

	std::vector<DirectX::XMINT4> ids(MaxInstances);

	m_instVisCount = 0;
	for (UINT i{}; i < m_instCount; ++i) {
		if (isBoxInside(frustum, m_modelBBs[i].vmin, m_modelBBs[i].vmax)) {
			ids[m_instVisCount++].x = i;
		}
	}

	D3D11_MAPPED_SUBRESOURCE subresource;
	HRESULT hr = m_pDeviceContext->Map(m_pModelBufferInstVis, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
	if (FAILED(hr)) {
		return;
	}

	memcpy(subresource.pData, ids.data(), sizeof(DirectX::XMINT4) * m_instVisCount);
	m_pDeviceContext->Unmap(m_pModelBufferInstVis, 0);
}
