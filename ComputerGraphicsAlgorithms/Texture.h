#pragma once

#include "framework.h"
#include "ComputerGraphicsAlgorithms.h"
#include "Renderer.h"

#include <string>

struct TextureDesc {
    UINT32 pitch{};
    UINT32 mipmapsCount{};

    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;

    UINT32 width{};
    UINT32 height{};

    void* pData{};
};

bool LoadDDS(const std::wstring& filepath, TextureDesc& desc, bool singleMip = false);
