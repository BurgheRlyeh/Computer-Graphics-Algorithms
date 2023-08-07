#include "ShaderProcessor.h"

HRESULT compileAndCreateShader(
	ID3D11Device* device,
	const std::wstring& path,
	ID3D11DeviceChild** ppShader,
	ID3DBlob** ppCode
) {
	// Try to load shader's source code first
	FILE* pFile{};
	_wfopen_s(&pFile, path.c_str(), L"rb");
	if (!pFile) {
		return E_FAIL;
	}

	fseek(pFile, 0, SEEK_END);
	long long size = _ftelli64(pFile);
	fseek(pFile, 0, SEEK_SET);

	std::vector<char> data;
	data.resize(size + 1);

	size_t rd{ fread(data.data(), 1, size, pFile) };
	assert(rd == (size_t)size);

	fclose(pFile);

	// Determine shader's type
	std::wstring ext{ Extension(path) };

	std::string entryPoint{ WCSToMBS(ext) };
	std::string platform{ entryPoint + "_5_0"};

	// Setup flags
	UINT flags{};
#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

	// Try to compile
	ID3DBlob* pCode{};
	ID3DBlob* pErrMsg{};
	HRESULT hr{ D3DCompile(
		data.data(),
		data.size(),
		WCSToMBS(path).c_str(),
		nullptr,
		nullptr,
		entryPoint.c_str(),
		platform.c_str(),
		flags,
		0,
		&pCode,
		&pErrMsg
	) };

	if (FAILED(hr)) {
		if (pErrMsg) {
			OutputDebugStringA(static_cast<const char*>(pErrMsg->GetBufferPointer()));
		}
		return hr;
	}
	SAFE_RELEASE(pErrMsg);

	// Create shader itself if anything else is OK
	if (ext == L"vs") {
		ID3D11VertexShader* pVertexShader{};
		hr = device->CreateVertexShader(
			pCode->GetBufferPointer(),
			pCode->GetBufferSize(),
			nullptr,
			&pVertexShader
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppShader = pVertexShader;
	} else if (ext == L"ps") {
		ID3D11PixelShader* pPixelShader{};
		hr = device->CreatePixelShader(
			pCode->GetBufferPointer(),
			pCode->GetBufferSize(),
			nullptr,
			&pPixelShader
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppShader = pPixelShader;
	}

	hr = SetResourceName(*ppShader, WCSToMBS(path).c_str());

	if (ppCode) {
		*ppCode = pCode;
	} else {
		SAFE_RELEASE(pCode);
	}

	return hr;
}