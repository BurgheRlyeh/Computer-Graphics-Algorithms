#include "Cube.h"

#include "ShaderProcessor.h"
#include "CubeBVH.h"

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

float Cube::getModelRotationSpeed() {
	return m_modelRotationSpeed;
}

bool Cube::getIsDoCull() {
	return m_doCull;
}

bool Cube::getIsRayTracing() {
	return m_isRayTracing;
}

int Cube::getInstCount() {
	return m_instCount;
}

HRESULT Cube::init(Matrix* positions, int num) {
	HRESULT hr{ S_OK };

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

	UINT16 indices[36]{
		 0,	 2,  1,  0,  3,  2,
		 4,	 6,  5,  4,  7,  6,
		 8,	10,  9,  8, 11, 10,
		12, 14, 13, 12, 15, 14,
		16, 18, 17, 16, 19, 18,
		20, 22, 21, 20, 23, 22
	};

	D3D11_INPUT_ELEMENT_DESC InputDesc[]{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// create vertex buffer
	{
		hr = createVertexBuffer(vertices, sizeof(vertices) / sizeof(*vertices));
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pVertexBuffer, "CubeVertexBuffer");
		THROW_IF_FAILED(hr);
	}

	// create index buffer
	{
		hr = createIndexBuffer(indices, sizeof(indices) / sizeof(*indices));
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pIndexBuffer, "CubeIndexBuffer");
		THROW_IF_FAILED(hr);
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
		THROW_IF_FAILED(hr);

		hr = compileAndCreateShader(
			m_pDevice,
			L"CubeShader.ps",
			(ID3D11DeviceChild**)&m_pPixelShader
		);
		THROW_IF_FAILED(hr);
	}
	
	// create input layout
	{
		hr = m_pDevice->CreateInputLayout(
			InputDesc,
			4,
			vertexShaderCode->GetBufferPointer(),
			vertexShaderCode->GetBufferSize(),
			&m_pInputLayout
		);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pInputLayout, "InputLayout");
		THROW_IF_FAILED(hr);
	}
	SAFE_RELEASE(vertexShaderCode);

	// create model buffer
	{
		hr = createModelBuffer();
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pModelBuffer, "ModelBufferInst");
		THROW_IF_FAILED(hr);
	}

	// create vertices & indices buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(VIBuffer) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		D3D11_SUBRESOURCE_DATA data{ &m_viBuffer, sizeof(VIBuffer) };

		hr = m_pDevice->CreateBuffer(&desc, &data, &m_pVIBuffer);
		THROW_IF_FAILED(hr);
	}

	// create model -1 buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(ModelBufferInv) * MaxInstances },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pModelBufferInv);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pModelBufferInv, "ModelBufferInv");
		THROW_IF_FAILED(hr);
	}

	// create bvh buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(CubeBVH::BVHConstBuf) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pBVHBuffer);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pBVHBuffer, "BVHBuffer");
		THROW_IF_FAILED(hr);
	}

	// init models
	{
		int useNM{ 1 };
		m_modelBuffers[0].settings = {
			0.0f, m_modelRotationSpeed, 0.0f, *reinterpret_cast<float*>(&useNM)
		};
		m_modelBuffers[0].posAndAng = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_modelBuffers[0].updateMatrices();
		m_modelBuffersInv[0].modelMatrixInv = m_modelBuffers[0].matrix.Invert();

		Vector4 diag{ sqrtf(2.0f), 1.f, sqrtf(2.0f), 0.0f };
		m_modelBBs[0].bmin = m_modelBuffers[0].posAndAng - diag / 2;
		m_modelBBs[0].bmax = m_modelBuffers[0].posAndAng + diag / 2;

		m_modelBuffers[1].settings = {
			64.0f, 0.0f, 0.0f, *reinterpret_cast<float*>(&useNM)
		};
		m_modelBuffers[1].posAndAng = { 2.0f, 0.0f, 0.0f, 0.0f };
		m_modelBuffers[1].updateMatrices();
		m_modelBuffersInv[1].modelMatrixInv = m_modelBuffers[1].matrix.Invert();

		m_modelBBs[1].bmin = m_modelBuffers[1].posAndAng - Vector4{ 0.5f, 0.5f, 0.5f, 0.0f };
		m_modelBBs[1].bmax = m_modelBuffers[1].posAndAng + Vector4{ 0.5f, 0.5f, 0.5f, 0.0f };

		m_updateCullParams = true;
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
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pModelBufferInstVis, "ModelBufferInstVis");
		THROW_IF_FAILED(hr);
	}

	// load texture and create view 
	{
		TextureDesc textureDesc[2];

		const std::wstring textureName{ L"../Common/Brick.dds" };

		bool ddsRes{ LoadDDS(textureName.c_str(), textureDesc[0]) };
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
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pTexture, WCSToMBS(L"Diffuse textures"));
		THROW_IF_FAILED(hr);

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
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pTexture, WCSToMBS(L"Diffuse texture views"));
		THROW_IF_FAILED(hr);

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
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pTextureNM, WCSToMBS(textureName));
		THROW_IF_FAILED(hr);

		hr = createResourceView(textureDesc, m_pTextureNM, &m_pTextureViewNM);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pTexture, "CubeTextureViewNM");
		THROW_IF_FAILED(hr);

		free(textureDesc.pData);
	}

	// timers init
	{
		m_pGPUTimer = new GPUTimer(m_pDevice, m_pDeviceContext);
		m_pGPUTimer->init();
		m_pCPUTimer = new CPUTimer();
	}

	for (int i{}; i < 24; ++i) {
		bvh_vertices[i] = m_viBuffer.vertices[i].point;
	}

	return hr;
}

HRESULT Cube::createVertexBuffer(Vertex(&vertices)[], UINT numVertices) {
	D3D11_BUFFER_DESC desc{
		.ByteWidth{ sizeof(Vertex) * numVertices },
		.Usage{ D3D11_USAGE_IMMUTABLE },
		.BindFlags{ D3D11_BIND_VERTEX_BUFFER }
	};

	D3D11_SUBRESOURCE_DATA data{
		.pSysMem{ vertices },
		.SysMemPitch{ sizeof(Vertex) * numVertices }
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
		.ByteWidth{ sizeof(ModelBuffer)* MaxInstances },
		.Usage{ D3D11_USAGE_DEFAULT },
		.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
	};

	return m_pDevice->CreateBuffer(&desc, nullptr, &m_pModelBuffer);
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
	SAFE_RELEASE(m_pModelBuffer);
	SAFE_RELEASE(m_pModelBufferInstVis);

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pTextureView);

	SAFE_RELEASE(m_pTextureNM);
	SAFE_RELEASE(m_pTextureViewNM);

	// Term GPU culling setup
	SAFE_RELEASE(m_pCullShader);
	SAFE_RELEASE(m_pIndirectArgsSrc);
	SAFE_RELEASE(m_pIndirectArgs);
	SAFE_RELEASE(m_pCullParams);
	SAFE_RELEASE(m_pIndirectArgsUAV);
	SAFE_RELEASE(m_pModelBufferInstVisGPU);
	SAFE_RELEASE(m_pModelBufferInstVisGPU_UAV);
	for (int i = 0; i < 10; i++) {
		SAFE_RELEASE(m_queries[i]);
	}
}

void Cube::update(float delta, bool isRotate) {
	if (!isRotate) {
		return;
	}

	for (int i{}; i < m_instCount; ++i) {
		m_modelBuffers[i].posAndAng.w += delta * m_modelBuffers[i].settings.y;
		m_modelBuffers[i].updateMatrices();
		m_modelBuffers[i].matrix.Invert(m_modelBuffersInv[i].modelMatrixInv);

		bvh_matrices[i] = m_modelBuffers[i].matrix;
	}

	m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, m_modelBuffers.data(), 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pModelBufferInv, 0, nullptr, m_modelBuffersInv.data(), 0, 0);

	updateBVH();
}

void Cube::updateBVH(bool rebuild) {
	m_pCPUTimer->start();
	
	if (m_isRebuildBVH || rebuild) {
		bvh.init(m_instCount, bvh_vertices, m_viBuffer.indices, bvh_matrices);
		bvh.build();
		m_isRebuildBVH = false;
	}
	else {
		bvh.update(m_instCount, bvh_vertices, m_viBuffer.indices, bvh_matrices);
	}
	
	m_pCPUTimer->stop();

	m_pDeviceContext->UpdateSubresource(m_pBVHBuffer, 0, nullptr, &bvh.m_bvhCBuf, 0, 0);
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

	ID3D11Buffer* cbuffers[]{ pSceneBuffer, m_pModelBuffer, m_pModelBufferInstVis };
	m_pDeviceContext->VSSetConstantBuffers(0, 3, cbuffers);
	m_pDeviceContext->PSSetConstantBuffers(0, 3, cbuffers);

	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

	if (m_doCull) {
		if (m_computeCull) {
			m_pDeviceContext->CopyResource(m_pIndirectArgs, m_pIndirectArgsSrc);
			m_pDeviceContext->Begin(m_queries[m_curFrame % 10]);
			m_pGPUTimer->start();
			m_pDeviceContext->DrawIndexedInstancedIndirect(m_pIndirectArgs, 0);
			m_pGPUTimer->stop();
			m_pDeviceContext->End(m_queries[m_curFrame++ % 10]);
		} else {
			m_pGPUTimer->start();
			m_pDeviceContext->DrawIndexedInstanced(36, m_instVisCount, 0, 0, 0);
			m_pGPUTimer->stop();
		}
	} else {
		m_pGPUTimer->start();
		m_pDeviceContext->DrawIndexedInstanced(36, m_instCount, 0, 0, 0);
		m_pGPUTimer->stop();
	}
}

void Cube::rayTracingInit(ID3D11Texture2D* texture) {
	// create shader
	HRESULT hr{ compileAndCreateShader(
		m_pDevice,
		L"RayCasting.cs",
		(ID3D11DeviceChild**)&m_pRTShader
	) };
	THROW_IF_FAILED(hr);

	// create
	rayTracingUpdate(texture);
}
	
void Cube::rayTracingUpdate(ID3D11Texture2D* tex) {
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
		.Format{ DXGI_FORMAT_UNKNOWN },
		.ViewDimension{ D3D11_UAV_DIMENSION_TEXTURE2D },
		.Texture2D{}
	};

	HRESULT hr{ 
		m_pDevice->CreateUnorderedAccessView(tex, &desc, &m_pRTTexture)
	};
	THROW_IF_FAILED(hr);
	 
	hr = SetResourceName(m_pRTTexture, "RayCastingTextureUAV");
	THROW_IF_FAILED(hr);
}

void Cube::rayTracing(ID3D11SamplerState* pSampler, ID3D11Buffer* m_pSBuf, ID3D11Buffer* m_pRTBuf, int w, int h) {
	ID3D11SamplerState* samplers[]{ pSampler };
	m_pDeviceContext->CSSetSamplers(0, 1, samplers);

	ID3D11ShaderResourceView* resources[]{ m_pTextureView, m_pTextureViewNM };
	m_pDeviceContext->CSSetShaderResources(0, 2, resources);

	ID3D11Buffer* constBuffers[6]{
		m_pSBuf, m_pModelBuffer, m_pVIBuffer, m_pRTBuf, m_pModelBufferInv, m_pBVHBuffer
	};
	m_pDeviceContext->CSSetConstantBuffers(0, 6, constBuffers);

	// unbind rtv
	ID3D11RenderTargetView* nullRtv{};
	m_pDeviceContext->OMSetRenderTargets(1, &nullRtv, nullptr);

	ID3D11UnorderedAccessView* uavBuffers[]{ m_pRTTexture };
	m_pDeviceContext->CSSetUnorderedAccessViews(0, 1, uavBuffers, nullptr);

	m_pDeviceContext->CSSetShader(m_pRTShader, nullptr, 0);
	m_pGPUTimer->start();
	m_pDeviceContext->Dispatch(w, h, 1);
	m_pGPUTimer->stop();

	// unbind uav
	ID3D11UnorderedAccessView* nullUav{};
	m_pDeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
}

HRESULT Cube::initCull() {
	HRESULT hr{ S_OK };

	// create shader
	hr = compileAndCreateShader(
		m_pDevice,
		L"FrustumCull.cs",
		(ID3D11DeviceChild**)&m_pCullShader
	);
	THROW_IF_FAILED(hr);

	// create indirect args buffer (for calculation)
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) },
			.Usage{ D3D11_USAGE_DEFAULT },
			// для unordered access
			.BindFlags{ D3D11_BIND_UNORDERED_ACCESS },
			// структурированный буфер
			.MiscFlags{ D3D11_RESOURCE_MISC_BUFFER_STRUCTURED },
			.StructureByteStride{ sizeof(UINT) }
		};

		hr = m_pDevice->CreateBuffer(
			&desc, nullptr, &m_pIndirectArgsSrc
		);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pIndirectArgsSrc, "IndirectArgsSrc");
		THROW_IF_FAILED(hr);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format{ DXGI_FORMAT_UNKNOWN },
			.ViewDimension{ D3D11_UAV_DIMENSION_BUFFER },
			.Buffer{ .NumElements{
				sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS)
					/ sizeof(UINT)
			} }
		};

		hr = m_pDevice->CreateUnorderedAccessView(
			m_pIndirectArgsSrc, &uavDesc, &m_pIndirectArgsUAV
		);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pIndirectArgsSrc, "IndirectArgsUAV");
		THROW_IF_FAILED(hr);
	}

	// create indirect args buffer (for usage)
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.MiscFlags{ D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS },
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pIndirectArgs);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pIndirectArgs, "IndirectArgs");
		THROW_IF_FAILED(hr);
	}

	// create culling params buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(CullParams) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pCullParams);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pCullParams, "CullParams");
	}

	// create output buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(XMINT4)* MaxInstances },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_UNORDERED_ACCESS },
			.MiscFlags{ D3D11_RESOURCE_MISC_BUFFER_STRUCTURED },
			.StructureByteStride{ sizeof(XMINT4) }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pModelBufferInstVisGPU);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pModelBufferInstVisGPU, "ModelBufferInstVisGPU");
		THROW_IF_FAILED(hr);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format{ DXGI_FORMAT_UNKNOWN },
			.ViewDimension{ D3D11_UAV_DIMENSION_BUFFER },
			.Buffer{ 0, MaxInstances, 0 }
		};

		hr = m_pDevice->CreateUnorderedAccessView(
			m_pModelBufferInstVisGPU, &uavDesc, &m_pModelBufferInstVisGPU_UAV
		);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pIndirectArgsSrc, "ModelBufferInstVisGPU_UAV");
		THROW_IF_FAILED(hr);
	}

	{
		D3D11_QUERY_DESC desc{ D3D11_QUERY_PIPELINE_STATISTICS };
		for (int i{}; i < 10 && SUCCEEDED(hr); ++i) {
			hr = m_pDevice->CreateQuery(&desc, &m_queries[i]);
		}
		THROW_IF_FAILED(hr);
	}

	return hr;
}

void Cube::calcFrustum(Camera& camera, float aspectRatio, Plane frustum[6]) {
	Vector3 dir{ -1.f * camera.getDir() };
	Vector3 up{ camera.getUp() };
	Vector3 right{ camera.getRight() };
	Vector3 pos{ camera.getPosition() };

	float n{ 0.1f };
	float f{ 100.0f };
	float fov{ XM_PI / 3 };

	float x{ n * tanf(fov / 2) };
	float y{ n * tanf(fov / 2) * aspectRatio };

	Vector3 nearVertices[4]{
		pos + dir * n - up * y - right * x,
		pos + dir * n - up * y + right * x,
		pos + dir * n + up * y + right * x,
		pos + dir * n + up * y - right * x
	};

	x = f * tanf(fov / 2);
	y = f * tanf(fov / 2) * aspectRatio;

	Vector3 farVertices[4]{
		pos + dir * f - up * y - right * x,
		pos + dir * f - up * y + right * x,
		pos + dir * f + up * y + right * x,
		pos + dir * f + up * y - right * x
	};

	frustum[0] = Plane(nearVertices[0], nearVertices[1], nearVertices[2]);
	frustum[1] = Plane(nearVertices[0], farVertices[0], farVertices[1]);
	frustum[2] = Plane(nearVertices[1], farVertices[1], farVertices[2]);
	frustum[3] = Plane(nearVertices[2], farVertices[2], farVertices[3]);
	frustum[4] = Plane(nearVertices[3], farVertices[3], farVertices[0]);
	frustum[5] = Plane(farVertices[1], farVertices[0], farVertices[3]);
}

void Cube::updateCullParams() {
	if (!m_updateCullParams) {
		return;
	}
	m_updateCullParams = false;

	CullParams cullParams{};
	cullParams.shapeCount = {
		static_cast<int>(m_instCount), 0, 0, 0
	};

	for (UINT i{}; i < m_instCount; ++i) {
		cullParams.bbMin[i] = {
			m_modelBBs[i].bmin.x,
			m_modelBBs[i].bmin.y,
			m_modelBBs[i].bmin.z,
			0.0f
		};
		cullParams.bbMax[i] = {
			m_modelBBs[i].bmax.x,
			m_modelBBs[i].bmax.y,
			m_modelBBs[i].bmax.z,
			0.0f
		};
	}

	m_pDeviceContext->UpdateSubresource(
		m_pCullParams, 0, nullptr, &cullParams, 0, 0
	);
}

// is box inside
bool isBoxInside(const Plane frustum[6], const Vector4& bbMin, const Vector4& bbMax) {
	for (int i{}; i < 6; ++i) {
		// выбираем ближайшую точку
		Vector4 p{
			signbit(frustum[i].x) ? bbMin.x : bbMax.x,
			signbit(frustum[i].y) ? bbMin.y : bbMax.y,
			signbit(frustum[i].z) ? bbMin.z : bbMax.z,
			1.f
		};
		// если снаружи - false
		if (p.Dot(frustum[i]) < 0.f) {
			return false;
		}
	}
	return true;
}

void Cube::cullBoxes(ID3D11Buffer* m_pSceneBuffer, Camera& camera, float aspectRatio) {
	if (m_computeCull) {
		D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args{ 36 };
		m_pDeviceContext->UpdateSubresource(
			m_pIndirectArgsSrc, 0, nullptr, &args, 0, 0
		);

		ID3D11Buffer* constBuffers[2]{ m_pSceneBuffer, m_pCullParams };
		m_pDeviceContext->CSSetConstantBuffers(0, 2, constBuffers);

		ID3D11UnorderedAccessView* uavBuffers[2]{
			m_pIndirectArgsUAV, m_pModelBufferInstVisGPU_UAV
		};
		m_pDeviceContext->CSSetUnorderedAccessViews(0, 2, uavBuffers, nullptr);

		m_pDeviceContext->CSSetShader(m_pCullShader, nullptr, 0);
		m_pDeviceContext->Dispatch((m_instCount - 1) / 64u + 1, 1, 1);
		m_pDeviceContext->CopyResource(m_pModelBufferInstVis, m_pModelBufferInstVisGPU);
		m_pDeviceContext->CopyResource(m_pIndirectArgs, m_pIndirectArgsSrc);
	} else {
		Plane frustum[6];
		calcFrustum(camera, aspectRatio, frustum);

		std::vector<XMINT4> ids(MaxInstances);

		m_instVisCount = 0;
		for (UINT i{}; i < m_instCount; ++i) {
			if (isBoxInside(frustum, m_modelBBs[i].bmin, m_modelBBs[i].bmax)) {
				ids[m_instVisCount++].x = i;
			}
		}

		D3D11_MAPPED_SUBRESOURCE subresource;
		HRESULT hr = m_pDeviceContext->Map(m_pModelBufferInstVis, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
		THROW_IF_FAILED(hr);

		memcpy(subresource.pData, ids.data(), sizeof(XMINT4) * m_instVisCount);
		m_pDeviceContext->Unmap(m_pModelBufferInstVis, 0);
	}
}

void Cube::readQueries() {
	D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;

	while (m_lastCompletedFrame < m_curFrame) {
		HRESULT hr = m_pDeviceContext->GetData(
			m_queries[m_lastCompletedFrame % 10],
			&stats,
			sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS),
			0
		);
		if (hr != S_OK) {
			break;
		}
		m_gpuVisibleInstances = (int)stats.IAPrimitives / 12;
		++m_lastCompletedFrame;
	}
}

void Cube::initImGUI() {
	ImGui::Begin("Instances");

	ImGui::Checkbox("Use ray tracing", &m_isRayTracing);

	bool add = ImGui::Button("+");
	ImGui::SameLine();
	bool remove = ImGui::Button("-");

	ImGui::Text("Count %d", m_instCount);
	if (m_computeCull) {
		ImGui::Text("Visible (GPU) %d", m_gpuVisibleInstances);
	} else {
		ImGui::Text("Visible %d", m_instVisCount);
	}
	m_doCull |= m_computeCull;
	ImGui::Checkbox("Cull", &m_doCull);
	if (m_doCull) {
		ImGui::Checkbox("Cull on GPU", &m_computeCull);
	}

	ImGui::End();

	if (add && m_instCount < MaxInstances) {
		Vector4 pos{ m_modelBuffers[m_instCount].posAndAng };
		if (!pos.x && !pos.y && !pos.z) {
			initModel(m_modelBuffers[m_instCount], m_modelBBs[m_instCount]);
			m_isRebuildBVH = true;
		}
		//m_modelBuffers[m_instCount].matrix.Invert(m_modelBuffersInv[m_instCount].modelMatrixInv);
		++m_instCount;
	}
	if (remove && m_instCount) {
		--m_instCount;
	}
	m_updateCullParams = add || remove;
}

void Cube::initModel(ModelBuffer& modelBuffer, AABB& bb) {
	bool kitty{ randNormf() > 0.5f };
	int useNM = static_cast<int>(!kitty);

	modelBuffer.settings = {
		1e2f * randNormf(), //> 0.5f ? 64.0f : 0.0f,
		XM_2PI * randNormf(),
		static_cast<float>(kitty) ,
		*reinterpret_cast<float*>(&useNM)
	};

	modelBuffer.posAndAng = {
		7.0f * randNormf() - 3.5f,
		7.0f * randNormf() - 3.5f,
		7.0f * randNormf() - 3.5f,
		0.f
	};

	Vector4 diag{ sqrtf(2.0f), 1.f, sqrtf(2.0f), 0.f };
	bb.bmin = modelBuffer.posAndAng - diag / 2;
	bb.bmax = modelBuffer.posAndAng + diag / 2;
}
