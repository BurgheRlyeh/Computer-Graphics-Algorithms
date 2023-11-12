#include "Geometry.h"

#include "ShaderProcessor.h"
#include "BVH.h"

#include "../Common/imgui/imgui.h"
#include "../Common/imgui/backends/imgui_impl_dx11.h"
#include "../Common/imgui/backends/imgui_impl_win32.h"

#include "CSVGeometryLoader.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void Geometry::ModelBuffer::updateMatrices() {
	Vector3 pos{ posAngle.x, posAngle.y, posAngle.z };
	float ang{ posAngle.w };

	mModel = Matrix::CreateRotationY(ang) * Matrix::CreateTranslation(pos);
	mModelInv = mModel.Invert();
	mNormal = mModelInv.Transpose();
}

float Geometry::getModelRotationSpeed() {
	return m_modelRotationSpeed;
}

bool Geometry::getIsRayTracing() {
	return m_isRayTracing;
}

HRESULT Geometry::init(ID3D11Texture2D* tex) {
	HRESULT hr{ S_OK };

	// upload geometry
	{
		auto geom = CSVGeometryLoader::loadFrom("11715.csv");
		std::copy(geom.indices.begin(), geom.indices.end(), m_viBuffer.indices);
		std::copy(geom.vertices.begin(), geom.vertices.end(), m_viBuffer.vertices);
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

		hr = SetResourceName(m_pVIBuffer, "VertexIndexBuffer");
		THROW_IF_FAILED(hr);
	}

	// create model buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(ModelBuffer) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pModelBuffer);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pModelBuffer, "ModelBuffer");
		THROW_IF_FAILED(hr);
	}

	// create aabb buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(BVH::bbs) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pAABBBuffer);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pAABBBuffer, "AABBBuffer");
		THROW_IF_FAILED(hr);
	}

	// create lfcp buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(BVH::lfcps) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pLFCPBuffer);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pLFCPBuffer, "LFCPBuffer");
		THROW_IF_FAILED(hr);
	}

	// create tri buffer
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth{ sizeof(BVH::triIdx) },
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER }
		};

		hr = m_pDevice->CreateBuffer(&desc, nullptr, &m_pTriBuffer);
		THROW_IF_FAILED(hr);

		hr = SetResourceName(m_pTriBuffer, "TriBuffer");
		THROW_IF_FAILED(hr);
	}

	// shader processing
	{
		hr = compileAndCreateShader(
			m_pDevice,
			L"RayTracing.cs",
			(ID3D11DeviceChild**)&m_pRayTracingCS
		);
		THROW_IF_FAILED(hr);
	}

	resizeUAV(tex);

	// timers init
	{
		m_pGPUTimer = new GPUTimer(m_pDevice, m_pDeviceContext);
		m_pGPUTimer->init();
		m_pCPUTimer = new CPUTimer();
	}

	return hr;
}

void Geometry::term() {
	SAFE_RELEASE(m_pModelBuffer);
	SAFE_RELEASE(m_pAABBBuffer);
	SAFE_RELEASE(m_pTriBuffer);
}

void Geometry::update(float delta, bool isRotate) {
	if (!isRotate) {
		return;
	}

	m_modelBuffer.posAngle.w += delta * m_modelBuffer.shineSpeedTexIdNM.y;
	m_modelBuffer.updateMatrices();

	m_pDeviceContext->UpdateSubresource(m_pModelBuffer, 0, nullptr, &m_modelBuffer, 0, 0);

	updateBVH();
}

void Geometry::updateBVH(bool rebuild) {
	m_pCPUTimer->start();

	if (m_isRebuildBVH || rebuild) {
		bvh.init(1, m_viBuffer.vertices, m_viBuffer.indices, Matrix::Identity);
		bvh.build();
		m_isRebuildBVH = false;
	}
	else {
		bvh.update(1, m_viBuffer.vertices, m_viBuffer.indices, Matrix::Identity);
	}

	m_pCPUTimer->stop();

	m_pDeviceContext->UpdateSubresource(m_pAABBBuffer, 0, nullptr, &bvh.bbs, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pLFCPBuffer, 0, nullptr, &bvh.lfcps, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pTriBuffer, 0, nullptr, &bvh.triIdx, 0, 0);
}

void Geometry::resizeUAV(ID3D11Texture2D* tex) {
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
		.Format{ DXGI_FORMAT_UNKNOWN },
		.ViewDimension{ D3D11_UAV_DIMENSION_TEXTURE2D },
		.Texture2D{}
	};

	THROW_IF_FAILED(m_pDevice->CreateUnorderedAccessView(tex, &desc, &m_pUAVTexture));
	THROW_IF_FAILED(SetResourceName(m_pUAVTexture, "UAVTexture"));
}

void Geometry::rayTracing(ID3D11Buffer* m_pSBuf, ID3D11Buffer* m_pRTBuf, int w, int h) {
	ID3D11Buffer* constBuffers[7]{
		m_pSBuf,
		m_pModelBuffer,
		m_pVIBuffer,
		m_pRTBuf,
		m_pAABBBuffer,
		m_pLFCPBuffer,
		m_pTriBuffer
	};
	m_pDeviceContext->CSSetConstantBuffers(0, 7, constBuffers);

	// unbind rtv
	ID3D11RenderTargetView* nullRtv{};
	m_pDeviceContext->OMSetRenderTargets(1, &nullRtv, nullptr);

	ID3D11UnorderedAccessView* uavBuffers[]{ m_pUAVTexture };
	m_pDeviceContext->CSSetUnorderedAccessViews(0, 1, uavBuffers, nullptr);

	m_pDeviceContext->CSSetShader(m_pRayTracingCS, nullptr, 0);
	m_pGPUTimer->start();
	m_pDeviceContext->Dispatch(w, h, 1);
	m_pGPUTimer->stop();

	// unbind uav
	ID3D11UnorderedAccessView* nullUav{};
	m_pDeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
}
