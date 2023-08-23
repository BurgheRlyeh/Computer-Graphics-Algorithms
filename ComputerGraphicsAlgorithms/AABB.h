#pragma once

#include "framework.h"

struct AABB {
    DirectX::XMFLOAT3 vmin{
        INFINITY, INFINITY, INFINITY
    };

    DirectX::XMFLOAT3 vmax{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };

    inline DirectX::XMFLOAT3 getVert(int idx) const {
        return {
            !(idx & 1) ? vmin.x : vmax.x,
            !(idx & 2) ? vmin.y : vmax.y,
            !(idx & 4) ? vmin.z : vmax.z
        };
    }
};
