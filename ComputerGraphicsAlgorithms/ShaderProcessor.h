#pragma once

#include "framework.h"

HRESULT compileAndCreateShader(
	ID3D11Device* device,
	const std::wstring& path,
	ID3D11DeviceChild** ppShader,
	ID3DBlob** ppCode = nullptr
);
