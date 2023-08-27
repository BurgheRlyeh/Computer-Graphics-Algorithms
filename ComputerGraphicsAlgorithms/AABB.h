#pragma once

#include "framework.h"

struct AABB {
    DirectX::SimpleMath::Vector3 vmin{
        (std::numeric_limits<float>::max)(),
        (std::numeric_limits<float>::max)(),
        (std::numeric_limits<float>::max)()
    };

    DirectX::SimpleMath::Vector3 vmax{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };

    inline DirectX::SimpleMath::Vector3 getVert(int idx) const {
        return {
            !(idx & 1) ? vmin.x : vmax.x,
            !(idx & 2) ? vmin.y : vmax.y,
            !(idx & 4) ? vmin.z : vmax.z
        };
    }
};
