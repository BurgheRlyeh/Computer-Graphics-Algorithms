#include "Cube.h"

#include "ShaderProcessor.h"

#include "../Common/imgui/imgui.h"
#include "../Common/imgui/backends/imgui_impl_dx11.h"
#include "../Common/imgui/backends/imgui_impl_win32.h"

#include "DDSTextureLoader.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void Cube::ModelBuffer::updateMatrices() {
	Vector3 pos{ posAndAng.x, posAndAng.y, posAndAng.z };
	float ang{ posAndAng.w };

	matrix = Matrix::CreateRotationY(ang) * Matrix::CreateTranslation(pos);
	normalMatrix = matrix.Invert().Transpose();
}

HRESULT Cube::init(Matrix* positions, int num) {
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
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pVertexBuffer, "CubeVertexBuffer");
		ThrowIfFailed(hr);
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
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pIndexBuffer, "CubeIndexBuffer");
		ThrowIfFailed(hr);
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
		ThrowIfFailed(hr);

		hr = compileAndCreateShader(
			m_pDevice,
			L"CubeShader.ps",
			(ID3D11DeviceChild**)&m_pPixelShader
		);
		ThrowIfFailed(hr);
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
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pInputLayout, "InputLayout");
		ThrowIfFailed(hr);
	}
	SAFE_RELEASE(vertexShaderCode);

	// create model buffer
	{
		hr = createModelBuffer();
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pModelBufferInst, "ModelBufferInst");
		ThrowIfFailed(hr);
	}

	// init models
	{
		const float diag = sqrtf(2.0f) / 2.0f * 0.9f;
		Vector3 diagonal{ diag, 0.5f, diag };

		Vector3 pos{ 0.0f, 0.0f, 0.0f };

		int useNM{ 1 };
		m_modelBuffers[0].settings = {
			0.0f, m_modelRotationSpeed, 0.0f, *reinterpret_cast<float*>(&useNM)
		};
		m_modelBuffers[0].posAndAng = { 1e-5f, 0.0f, 0.0f, 0.0f };
		m_modelBBs[0].vmin = pos - diagonal;
		m_modelBBs[0].vmax = pos + diagonal;


		pos = { 2.0f, 0.0f, 0.0f };

		m_modelBuffers[1].settings = {
			64.0f, 0.0f, 0.0f, *reinterpret_cast<float*>(&useNM)
		};
		m_modelBuffers[1].posAndAng = { pos.x, pos.y, pos.z, 0.0f };
		m_modelBuffers[1].updateMatrices();
		m_modelBBs[1].vmin = pos - Vector3{ 0.5f, 0.5f, 0.5f };
		m_modelBBs[1].vmax = pos + Vector3{ 0.5f, 0.5f, 0.5f };

		/*for (int i{ 2 }; i < 10; ++i) {
			initModel(m_modelBuffers[i], m_modelBBs[i]);
		}
		m_instCount = 10;*/
	}

	// create model visibility buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(XMINT4) * MaxInstances },
			.Usage{ D3D11_USAGE_DYNAMIC },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER },
			.CPUAccessFlags{ D3D11_CPU_ACCESS_WRITE }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pModelBufferInstVis);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pModelBufferInstVis, "ModelBufferInstVis");
		ThrowIfFailed(hr);
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
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pTexture, WCSToMBS(L"Diffuse textures"));
		ThrowIfFailed(hr);

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
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pTexture, WCSToMBS(L"Diffuse texture views"));
		ThrowIfFailed(hr);

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
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pTextureNM, WCSToMBS(textureName));
		ThrowIfFailed(hr);

		hr = createResourceView(textureDesc, m_pTextureNM, &m_pTextureViewNM);
		ThrowIfFailed(hr);

		hr = SetResourceName(m_pTexture, "CubeTextureViewNM");
		ThrowIfFailed(hr);

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

void Cube::update(float delta, bool isRotate) {
	if (!isRotate) {
		return;
	}

	for (auto& model : m_modelBuffers) {
		model.posAndAng.w += delta * model.settings.y;
		model.updateMatrices();
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
		Vector4 pos{ m_modelBuffers[m_instCount].posAndAng };
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
	bool kitty{ randNormf() > 0.5f };
	modelBuffer.settings = {
		randNormf() > 0.5f ? 64.0f : 0.0f,
		XM_2PI * randNormf(),
		kitty ? 1.0f : 0.0f,
		*reinterpret_cast<float*>(&kitty)
	};

	Vector3 pos{
		7.0f * randNormf() - 3.5f,
		7.0f * randNormf() - 3.5f,
		7.0f * randNormf() - 3.5f
	};
	modelBuffer.posAndAng = { pos.x, pos.y, pos.z, 0.0f };

	const float diag = sqrtf(2.0f) / 2.0f * 0.5f;
	Vector3 diagonal{ diag, 0.5f, diag };
	bb.vmin = pos - diagonal;
	bb.vmax = pos + diagonal;
}

// is box inside
bool isBoxInside(
	const Plane frustum[6],
	const Vector3& bbMin,
	const Vector3& bbMax
) {
	for (int i{}; i < 6; ++i) {
		Vector4 p{
			signbit(frustum[i].x) ? bbMin.x : bbMax.x,
			signbit(frustum[i].y) ? bbMin.y : bbMax.y,
			signbit(frustum[i].z) ? bbMin.z : bbMax.z,
			1.0f
		};
		if (p.Dot(frustum[i]) < 0.f) {
			return false;
		}
	}
	return true;
}

void Cube::cullBoxes(Camera& camera, float aspectRatio) {
	Vector3 dir{ -1.f * camera.getDir() };
	Vector3 up{ camera.getUp() };
	Vector3 right{ camera.getRight() };
	Vector3 pos{ camera.getPosition() };

	float n{ 0.1f };
	float f{ 100.0f };
	float fov{ XM_PI / 3 };

	float x{ n * tanf(fov / 2) };
	float y{ n * tanf(fov / 2) * aspectRatio };

	Vector3 nearVertices[3]{
		pos + dir * n - up * y - right * x,
		pos + dir * n - up * y + right * x,
		pos + dir * n + up * y + right * x
	};

	x = f * tanf(fov / 2);
	y = f * tanf(fov / 2) * aspectRatio;

	Vector3 farVertices[3]{
		pos + dir * f - up * y - right * x,
		pos + dir * f + up * y - right * x,
		pos + dir * f + up * y + right * x
	};

	Plane frustum[6]{
		Plane(nearVertices[0], nearVertices[1], nearVertices[2]),
		Plane(nearVertices[0],  farVertices[0], nearVertices[1]),
		Plane(nearVertices[2], nearVertices[1],  farVertices[2]),
		Plane(nearVertices[2],  farVertices[2],  farVertices[1]),
		Plane(nearVertices[0],  farVertices[1],  farVertices[0]),
		Plane(farVertices[2],  farVertices[0],  farVertices[1])
	};

	std::vector<XMINT4> ids(MaxInstances);

	m_instVisCount = 0;
	for (UINT i{}; i < m_instCount; ++i) {
		if (isBoxInside(frustum, m_modelBBs[i].vmin, m_modelBBs[i].vmax)) {
			ids[m_instVisCount++].x = i;
		}
	}

	D3D11_MAPPED_SUBRESOURCE subresource;
	HRESULT hr = m_pDeviceContext->Map(m_pModelBufferInstVis, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
	ThrowIfFailed(hr);

	memcpy(subresource.pData, ids.data(), sizeof(XMINT4) * m_instVisCount);
	m_pDeviceContext->Unmap(m_pModelBufferInstVis, 0);
}
