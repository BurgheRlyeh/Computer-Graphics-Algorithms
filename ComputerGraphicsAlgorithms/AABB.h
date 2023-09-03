#pragma once

#include "framework.h"

struct AABB {
    DirectX::SimpleMath::Vector3 vmin{
        INFINITY, INFINITY, INFINITY
    };

    DirectX::SimpleMath::Vector3 vmax{
        -INFINITY, -INFINITY, -INFINITY
    };

    inline DirectX::SimpleMath::Vector3 getVert(int idx) const {
        return {
            !(idx & 1) ? vmin.x : vmax.x,
            !(idx & 2) ? vmin.y : vmax.y,
            !(idx & 4) ? vmin.z : vmax.z
        };
    }
};
