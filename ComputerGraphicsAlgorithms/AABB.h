#pragma once

#include <limits>

#include "framework.h"

#undef min
#undef max

struct AABB {
    DirectX::SimpleMath::Vector4 bmin{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        0
    };

    DirectX::SimpleMath::Vector4 bmax{
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::min(),
        0
    };

    inline DirectX::SimpleMath::Vector4 getVert(int idx) const {
        return {
            idx & 1 ? bmax.x : bmin.x,
            idx & 2 ? bmax.y : bmin.y,
            idx & 4 ? bmax.z : bmin.z,
            0
        };
    }
};
