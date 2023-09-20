#include "ShaderProcessor.h"

class D3DInclude : public ID3DInclude {
	STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) {
		FILE* pFile = nullptr;
		fopen_s(&pFile, pFileName, "rb");
		assert(pFile != nullptr);
		if (pFile == nullptr) {
			return E_FAIL;
		}

		fseek(pFile, 0, SEEK_END);
		long long size = _ftelli64(pFile);
		fseek(pFile, 0, SEEK_SET);

		VOID* pData = malloc(size);
		if (pData == nullptr) {
			fclose(pFile);
			return E_FAIL;
		}

		size_t rd = fread(pData, 1, size, pFile);
		assert(rd == (size_t)size);

		if (rd != (size_t)size) {
			fclose(pFile);
			free(pData);
			return E_FAIL;
		}

		*ppData = pData;
		*pBytes = (UINT)size;

		return S_OK;
	}
	STDMETHOD(Close)(THIS_ LPCVOID pData) {
		free(const_cast<void*>(pData));
		return S_OK;
	}
};

HRESULT compileAndCreateShader(
	ID3D11Device* device,
	const std::wstring& path,
	ID3D11DeviceChild** ppShader,
	const std::vector<std::string>& defines,
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

	D3DInclude includeHandler;

	std::vector<D3D_SHADER_MACRO> shaderDefines;
	shaderDefines.resize(defines.size() + 1);
	for (int i = 0; i < defines.size(); i++) {
		shaderDefines[i].Name = defines[i].c_str();
		shaderDefines[i].Definition = "";
	}
	shaderDefines.back().Name = nullptr;
	shaderDefines.back().Definition = nullptr;

	// Try to compile
	ID3DBlob* pCode{};
	ID3DBlob* pErrMsg{};
	HRESULT hr{ D3DCompile(
		data.data(),
		data.size(),
		WCSToMBS(path).c_str(),
		shaderDefines.data(),
		&includeHandler,
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
	} else if (ext == L"cs") {
		ID3D11ComputeShader* pComputeShader{};
		hr = device->CreateComputeShader(
			pCode->GetBufferPointer(),
			pCode->GetBufferSize(),
			nullptr,
			&pComputeShader
		);
		THROW_IF_FAILED(hr);
		*ppShader = pComputeShader;
	}

	hr = SetResourceName(*ppShader, WCSToMBS(path).c_str());

	if (ppCode) {
		*ppCode = pCode;
	} else {
		SAFE_RELEASE(pCode);
	}

	return hr;
}