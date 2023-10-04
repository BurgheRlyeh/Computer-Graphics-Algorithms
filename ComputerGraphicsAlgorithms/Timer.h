#pragma once

#include <chrono>

#include "framework.h"

using namespace std;
using namespace std::chrono;

class Timer {
public:
	virtual void start() = 0;
	virtual void stop() = 0;

	virtual double getTime() = 0;
};

class CPUTimer : Timer {
	steady_clock::time_point m_timeStart{};
	steady_clock::time_point m_timeStop{};

public:
	void start() { m_timeStart = high_resolution_clock::now(); }
	void stop() { m_timeStop = high_resolution_clock::now(); }

	double getTime() {
		return duration<double, milli>(m_timeStop - m_timeStart).count();
	}
};

class GPUTimer : Timer {
	ID3D11Device* m_pDevice{};
	ID3D11DeviceContext* m_pDeviceContext{};

	ID3D11Query* m_pTimeStart{};
	ID3D11Query* m_pTimeStop{};
	ID3D11Query* m_pDisjoint{};

public:
	GPUTimer() = delete;
	GPUTimer(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext) :
		m_pDevice(pDevice),
		m_pDeviceContext(pDeviceContext)
	{}

	~GPUTimer() {
		SAFE_RELEASE(m_pTimeStart);
		SAFE_RELEASE(m_pTimeStop);
		SAFE_RELEASE(m_pDisjoint);
	}

	void init() {
		D3D11_QUERY_DESC desc{ D3D11_QUERY_TIMESTAMP };
		THROW_IF_FAILED(m_pDevice->CreateQuery(&desc, &m_pTimeStart));
		THROW_IF_FAILED(m_pDevice->CreateQuery(&desc, &m_pTimeStop));

		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		THROW_IF_FAILED(m_pDevice->CreateQuery(&desc, &m_pDisjoint));
	}

	void start() {
		m_pDeviceContext->Begin(m_pDisjoint);
		m_pDeviceContext->End(m_pTimeStart);
	}

	void stop() {
		m_pDeviceContext->End(m_pTimeStop);
		m_pDeviceContext->End(m_pDisjoint);
	}

	double getTime() {
		UINT64 t0{};
		while (m_pDeviceContext->GetData(m_pTimeStart, &t0, sizeof(t0), 0));

		UINT64 t{};
		while (m_pDeviceContext->GetData(m_pTimeStop, &t, sizeof(t), 0));

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
		while (m_pDeviceContext->GetData(
			m_pDisjoint, &disjoint, sizeof(disjoint), 0
		));

		return disjoint.Disjoint ? 0.0 : 1e3 * (t - t0) / disjoint.Frequency;
	}
};
