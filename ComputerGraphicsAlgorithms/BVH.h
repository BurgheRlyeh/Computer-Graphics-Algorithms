#pragma once

#include "framework.h"

#include "AABB.h"

struct AABB;

using namespace DirectX;
using namespace DirectX::SimpleMath;

#undef min
#undef max

#define MaxSteps 32

#define LIMIT_V 1013
#define LIMIT_I 1107

class BVH {
	struct Tri {
		Vector4 v0{}, v1{}, v2{};
		Vector4 center{};
	};

	/*struct BVHNode {
		AABB bb{};
		XMINT4 leftFirstCntParent;
	};*/

	static const INT N{ LIMIT_I };

	Tri tri[N]{};

	INT rootNodeIdx{};

public:
	INT cnt{};
	INT nodesUsed{ 1 };
	INT leafs{};
	INT depthMin{ 2 * N };
	INT depthMax{ -1 };

	// dichotomy settings
	INT trianglesPerLeaf{ 2 };

	// sah settings
	bool isSAH{ true };
	bool isStepSAH{ true };
	bool isBinsSAH{ true };	// true / false
	INT sahStep{ 3 };

	//struct BVHConstBuf {
	//	BVHNode bvhNode[2 * N - 1]{};
	//} m_bvhCBuf;
	AABB bbs[2 * N - 1 - 165]{};
	XMINT4 lfcps[2 * N - 1]{};	// leftFirstCntParent
	XMINT4 triIdx[N]{};

	void init(INT instCnt, Vector4(&vertices)[LIMIT_V], XMINT4(&indices)[LIMIT_I], Matrix modelMatrices) {
		cnt = instCnt;
		nodesUsed = 1;
		leafs = 0;
		//trianglesPerLeaf = 0;
		depthMax = -1;
		depthMin = 2 * N;

		for (INT m{}; m < instCnt; ++m) {
			for (INT tr{}; tr < LIMIT_I; ++tr) {
				INT id{ LIMIT_I * m + tr };
				XMINT4 trIds{ indices[tr] };

				tri[id].v0 = Vector4::Transform(vertices[trIds.x], modelMatrices);
				tri[id].v1 = Vector4::Transform(vertices[trIds.y], modelMatrices);
				tri[id].v2 = Vector4::Transform(vertices[trIds.z], modelMatrices);

				tri[id].center = (tri[id].v0 + tri[id].v1 + tri[id].v2) / 3.f;

				triIdx[id] = { id, 0, 0, 0 };
			}
		}
	}

	void update(INT instCnt, Vector4(&vertices)[LIMIT_V], XMINT4(&indices)[LIMIT_I], Matrix modelMatrices) {
		for (INT m{}; m < instCnt; ++m) {
			for (INT tr{}; tr < LIMIT_I; ++tr) {
				INT id{ LIMIT_I * m + tr };
				XMINT4 trIds{ indices[tr] };

				tri[id].v0 = Vector4::Transform(vertices[trIds.x], modelMatrices);
				tri[id].v1 = Vector4::Transform(vertices[trIds.y], modelMatrices);
				tri[id].v2 = Vector4::Transform(vertices[trIds.z], modelMatrices);
				tri[id].center = (tri[id].v0 + tri[id].v1 + tri[id].v2) / 3.f;
			}
		}

		for (int i{ nodesUsed - 1 }; 0 <= i; --i) {
			//BVHNode& node = m_bvhCBuf.bvhNode[i];
			AABB& aabb = bbs[i];
			XMINT4& lfcp = lfcps[i];

			if (lfcp.z != 0) {
				updNodeBounds(i);
				continue;
			}

			AABB& l = bbs[lfcp.x];
			AABB& r = bbs[lfcp.x + 1];
			
			aabb.bmin = Vector4::Min(l.bmin, r.bmin);
			aabb.bmax = Vector4::Max(l.bmax, r.bmax);
		}
	}

	void build() {
		lfcps[0] = {
			0, 0, LIMIT_I * cnt, -1
		};
		updNodeBounds(rootNodeIdx);
		// subdivide recursively
		Subdivide(rootNodeIdx);
	}

private:
	void updNodeBounds(INT nodeIdx) {
		bbs[nodeIdx] = {};
		INT first{ lfcps[nodeIdx].y };
		for (INT i{}; i < lfcps[nodeIdx].z; ++i) {
			Tri& leafTri = tri[triIdx[first + i].x];

			bbs[nodeIdx].grow(leafTri.v0);
			bbs[nodeIdx].grow(leafTri.v1);
			bbs[nodeIdx].grow(leafTri.v2);
		}
	}

	float vecCompByIdx(Vector4 v, INT idx) {
		switch (idx) {
		case 0: return v.x;
		case 1: return v.y;
		case 2: return v.z;
		case 3: return v.w;
		default: throw;
		}
	}

	int depth(INT id) {
		INT d{ 0 };
		while (id != 0) {
			id = lfcps[id].w;
			++d;
		}
		return d;
	}

	void updDepths(INT id) {
		INT d = depth(id);
		if (d < depthMin)
			depthMin = d;
		if (d > depthMax)
			depthMax = d;
	}

	void splitDichotomy(INT nodeId, int& axis, float& splitPos) {
		Vector4 e{ bbs[nodeId].extent()};
		axis = static_cast<int>(e.x < e.y);
		axis += static_cast<int>(vecCompByIdx(e, axis) < e.z);
		splitPos = vecCompByIdx(bbs[nodeId].bmin + e / 2.f, axis);
	}

	float evaluateSAH(INT nodeId, int axis, float pos) {
		AABB leftBox{}, rightBox{};
		int leftCnt{}, rightCnt{};
		for (int i{}; i < lfcps[nodeId].z; ++i) {
			Tri& t = tri[triIdx[lfcps[nodeId].y + i].x];

			if (vecCompByIdx(t.center, axis) < pos) {
				++leftCnt;
				leftBox.grow(t.v0);
				leftBox.grow(t.v1);
				leftBox.grow(t.v2);
			}
			else {
				++rightCnt;
				rightBox.grow(t.v0);
				rightBox.grow(t.v1);
				rightBox.grow(t.v2);
			}
		}
		float cost{ leftCnt * leftBox.area() + rightCnt * rightBox.area() };
		return cost > 0 ? cost : std::numeric_limits<float>::max();
	}

	float splitSAH(INT nodeId, int& axis, float& splitPos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			for (int i{}; i < lfcps[nodeId].z; ++i) {
				Tri& t = tri[triIdx[lfcps[nodeId].y + i].x];
				Vector4 center{ t.center };
				float pos = vecCompByIdx(center, a);
				float cost = evaluateSAH(nodeId, a, pos);
				if (cost < bestCost) {
					axis = a;
					splitPos = pos;
					bestCost = cost;
				}
			}
		}

		return bestCost;
	}

	float splitStepSAH(INT nodeId, int& axis, float& pos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			float bmin{ vecCompByIdx(bbs[nodeId].bmin, a)};
			float bmax{ vecCompByIdx(bbs[nodeId].bmax, a) };
			if (bmin == bmax)
				continue;

			float step{ (bmax - bmin) / sahStep };
			for (int i{ 1 }; i < sahStep; ++i) {
				float candPos{ bmin + i * step };
				float cost = evaluateSAH(nodeId, a, candPos);
				if (cost < bestCost) {
					axis = a;
					pos = candPos;
					bestCost = cost;
				}
			}
		}

		return bestCost;
	}


	float splitDynamicStepSAH(INT nodeId, int& axis, float& splitPos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			float bmin{ vecCompByIdx(bbs[nodeId].bmin, a) };
			float bmax{ vecCompByIdx(bbs[nodeId].bmax, a) };
			if (bmin == bmax)
				continue;

			AABB bounds[MaxSteps]{};
			int triCnt[MaxSteps]{};

			float step = sahStep / (bmax - bmin);
			for (int i{}; i < lfcps[nodeId].z; ++i) {
				Tri& t = tri[triIdx[lfcps[nodeId].y + i].x];
				int id{ (std::min)(
					sahStep - 1,
					static_cast<int>((vecCompByIdx(t.center, a) - bmin) * step)
				) };
				++triCnt[id];
				bounds[id].grow(t.v0);
				bounds[id].grow(t.v1);
				bounds[id].grow(t.v2);
			}

			float lArea[MaxSteps - 1]{}, rArea[MaxSteps - 1]{};
			int lCnt[MaxSteps - 1]{}, rCnt[MaxSteps - 1]{};
			AABB lBox{}, rBox{};
			int lSum{}, rSum{};

			for (int i{}; i < sahStep - 1; ++i) {
				lSum += triCnt[i];
				lCnt[i] = lSum;
				lBox.grow(bounds[i]);
				lArea[i] = lBox.area();

				rSum += triCnt[sahStep - 1 - i];
				rCnt[sahStep - 2 - i] = rSum;
				rBox.grow(bounds[sahStep - 1 - i]);
				rArea[sahStep - 2 - i] = rBox.area();
			}
			step = (bmax - bmin) / sahStep;
			for (int i{}; i < sahStep - 1; ++i) {
				float planeCost{ lCnt[i] * lArea[i] + rCnt[i] * rArea[i] };
				if (planeCost < bestCost) {
					axis = a;
					splitPos = bmin + (i + 1) * step;
					bestCost = planeCost;
				}
			}
		}
		return bestCost;
	}

	void Subdivide(INT nodeId) {
		// determine split axis and position
		int axis{};
		float splitPos{};

		if (isSAH) {
			float cost;
			if (isStepSAH) {
				cost = isBinsSAH ? splitDynamicStepSAH(nodeId, axis, splitPos) : splitStepSAH(nodeId, axis, splitPos);
			}
			else {
				cost = splitSAH(nodeId, axis, splitPos);
			}

			if (cost >= bbs[nodeId].area() * lfcps[nodeId].z) {
				++leafs;
				updDepths(nodeId);
				return;
			}
		}
		else {
			if (lfcps[nodeId].z <= trianglesPerLeaf) {
				++leafs;
				updDepths(nodeId);
				return;
			}
			splitDichotomy(nodeId, axis, splitPos);
		}

		// in-place partition
		INT i{ lfcps[nodeId].y };
		INT j{ i + lfcps[nodeId].z - 1 };
		while (i <= j) {
			Tri& t = tri[triIdx[i++].x];
			Vector4 center{ t.center };

			if (splitPos <= vecCompByIdx(center, axis))
				std::swap(triIdx[--i].x, triIdx[j--].x);
		}

		// abort split if one of the sides is empty
		INT leftCnt{ i - lfcps[nodeId].y };
		if (leftCnt == 0 || leftCnt == lfcps[nodeId].z) {
			++leafs;
			updDepths(nodeId);
			return;
		}

		// create child nodes
		int leftIdx{ nodesUsed++ };
		lfcps[leftIdx] = {
			-1, lfcps[nodeId].y, leftCnt, nodeId
		};
		updNodeBounds(leftIdx);

		int rightIdx{ nodesUsed++ };
		lfcps[rightIdx] = {
			-1, i, lfcps[nodeId].z - leftCnt, nodeId
		};
		updNodeBounds(rightIdx);

		lfcps[nodeId] = {
			leftIdx, -1, 0, lfcps[nodeId].w
		};

		// recurse
		Subdivide(leftIdx);
		Subdivide(rightIdx);
	}
};