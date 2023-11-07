#pragma once

#include "framework.h"

#include "Cube.h"
#include "AABB.h"

class Cube;
struct AABB;

using namespace DirectX;
using namespace DirectX::SimpleMath;

#undef min
#undef max

#define MaxSteps 25

class BVH {
	struct Tri {
		Vector4 v0{}, v1{}, v2{};
		Vector4 center{};
	};

	struct BVHNode {
		AABB bb{};
		XMINT4 leftFirstCntParent;
	};

	static const INT N{ 300 };

	Tri tri[N]{};
	
	INT rootNodeIdx{};

public:
	INT cnt{};
	INT nodesUsed{ 1 };
	INT leafs{};
	INT depthMin{ 600 };
	INT depthMax{ -1 };

	// dichotomy settings
	INT trianglesPerLeaf{ 2 };

	// sah settings
	bool isSAH{ true };
	bool isStepSAH{ true };
	bool isBinsSAH{ true };	// true / false
	INT sahStep{ 8 };

	struct BVHConstBuf {
		BVHNode bvhNode[2 * N - 1]{};
		XMINT4 triIdx[N]{};
	} m_bvhCBuf;

	void init(INT instCnt, Vector4(&vertices)[24], XMINT4(&indices)[12], Matrix(&modelMatrices)[25]) {
		cnt = instCnt;
		nodesUsed = 1;
		leafs = 0;
		//trianglesPerLeaf = 0;
		depthMax = -1;
		depthMin = 599;

		for (INT m{}; m < instCnt; ++m) {
			for (INT tr{}; tr < 12; ++tr) {
				INT id{ 12 * m + tr };
				XMINT4 trIds{ indices[tr] };

				tri[id].v0 = Vector4::Transform(vertices[trIds.x], modelMatrices[m]);
				tri[id].v1 = Vector4::Transform(vertices[trIds.y], modelMatrices[m]);
				tri[id].v2 = Vector4::Transform(vertices[trIds.z], modelMatrices[m]);

				tri[id].center = (tri[id].v0 + tri[id].v1 + tri[id].v2) / 3.f;

				m_bvhCBuf.triIdx[id] = { id, 0, 0, 0 };
			}
		}
	}

	void update(INT instCnt, Vector4(&vertices)[24], XMINT4(&indices)[12], Matrix(&modelMatrices)[25]) {
		for (INT m{}; m < instCnt; ++m) {
			for (INT tr{}; tr < 12; ++tr) {
				INT id{ 12 * m + tr };
				XMINT4 trIds{ indices[tr] };

				tri[id].v0 = Vector4::Transform(vertices[trIds.x], modelMatrices[m]);
				tri[id].v1 = Vector4::Transform(vertices[trIds.y], modelMatrices[m]);
				tri[id].v2 = Vector4::Transform(vertices[trIds.z], modelMatrices[m]);
				tri[id].center = (tri[id].v0 + tri[id].v1 + tri[id].v2) / 3.f;
			}
		}

		for (int i{ nodesUsed - 1 }; 0 <= i; --i) {
			BVHNode& node = m_bvhCBuf.bvhNode[i];
			if (node.leftFirstCntParent.z != 0) {
				updNodeBounds(i);
				continue;
			}
			BVHNode& l = m_bvhCBuf.bvhNode[node.leftFirstCntParent.x];
			BVHNode& r = m_bvhCBuf.bvhNode[node.leftFirstCntParent.x + 1];
			node.bb.bmin = Vector4::Min(l.bb.bmin, r.bb.bmin);
			node.bb.bmax = Vector4::Max(l.bb.bmax, r.bb.bmax);
		}
	}

	void build() {
		BVHNode& root = m_bvhCBuf.bvhNode[rootNodeIdx];
		root.leftFirstCntParent = {
			0, 0, 12 * cnt, -1
		};
		updNodeBounds(rootNodeIdx);
		// subdivide recursively
		Subdivide(rootNodeIdx);
	}

private:
	void updNodeBounds(INT nodeIdx) {
		BVHNode& node = m_bvhCBuf.bvhNode[nodeIdx];
		node.bb = {};
		INT first{ node.leftFirstCntParent.y };
		for (INT i{}; i < node.leftFirstCntParent.z; ++i) {
			Tri& leafTri = tri[m_bvhCBuf.triIdx[first + i].x];

			node.bb.grow(leafTri.v0);
			node.bb.grow(leafTri.v1);
			node.bb.grow(leafTri.v2);
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
			id = m_bvhCBuf.bvhNode[id].leftFirstCntParent.w;
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

	void splitDichotomy(BVHNode& node, int& axis, float& splitPos) {
		Vector4 e{ node.bb.extent()};
		axis = static_cast<int>(e.x < e.y);
		axis += static_cast<int>(vecCompByIdx(e, axis) < e.z);
		splitPos = vecCompByIdx(node.bb.bmin + e / 2.f, axis);
	}

	float evaluateSAH(BVHNode& node, int axis, float pos) {
		AABB leftBox{}, rightBox{};
		int leftCnt{}, rightCnt{};
		for (int i{}; i < node.leftFirstCntParent.z; ++i) {
			Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];

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

	float splitSAH(BVHNode& node, int& axis, float& splitPos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			for (int i{}; i < node.leftFirstCntParent.z; ++i) {
				Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];
				Vector4 center{ t.center };
				float pos = vecCompByIdx(center, a);
				float cost = evaluateSAH(node, a, pos);
				if (cost < bestCost) {
					axis = a;
					splitPos = pos;
					bestCost = cost;
				}
			}
		}

		return bestCost;
	}

	float splitStepSAH(BVHNode& node, int& axis, float& pos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			float bmin{ vecCompByIdx(node.bb.bmin, a) };
			float bmax{ vecCompByIdx(node.bb.bmax, a) };
			/*for (int i{}; i < node.leftFirstCntParent.z; ++i) {
				Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];
				bmin = min(bmin, vecCompByIdx(t.center, a));
				bmax = max(bmax, vecCompByIdx(t.center, a));
			}*/

			if (bmin == bmax)
				continue;

			float step{ (bmax - bmin) / sahStep };
			for (int i{ 1 }; i < sahStep; ++i) {
				float candPos{ bmin + i * step };
				float cost = evaluateSAH(node, a, candPos);
				if (cost < bestCost) {
					axis = a;
					pos = candPos;
					bestCost = cost;
				}
			}
		}

		return bestCost;
	}


	float splitDynamicStepSAH(BVHNode& node, int& axis, float& splitPos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			float bmin{ vecCompByIdx(node.bb.bmin, a) };
			float bmax{ vecCompByIdx(node.bb.bmax, a) };
			if (bmin == bmax)
				continue;

			AABB bounds[MaxSteps]{};
			int triCnt[MaxSteps]{};

			float step = sahStep / (bmax - bmin);
			for (int i{}; i < node.leftFirstCntParent.z; ++i) {
				Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];
				int id{ min(
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
		// terminate recursion
		BVHNode& node{ m_bvhCBuf.bvhNode[nodeId] };

		// determine split axis and position
		int axis{};
		float splitPos{};

		if (isSAH) {
			float cost;

			if (isStepSAH) {
				cost = isBinsSAH ? splitDynamicStepSAH(node, axis, splitPos) : splitStepSAH(node, axis, splitPos);
			}
			else {
				cost = splitSAH(node, axis, splitPos);
			}

			//float cost = isStepSAH ? splitByStepSAH(node, axis, splitPos) : splitSAH(node, axis, splitPos);
			
			if (cost >= node.bb.area() * node.leftFirstCntParent.z) {
				++leafs;
				updDepths(nodeId);
				return;
			}
		}
		else {
			if (node.leftFirstCntParent.z <= trianglesPerLeaf) {
				++leafs;
				updDepths(nodeId);
				return;
			}
			splitDichotomy(node, axis, splitPos);
		}

		// in-place partition
		INT i{ node.leftFirstCntParent.y };
		INT j{ i + node.leftFirstCntParent.z - 1 };
		while (i <= j) {
			Tri& t = tri[m_bvhCBuf.triIdx[i++].x];
			Vector4 center{ t.center };

			if (splitPos <= vecCompByIdx(center, axis))
				std::swap(m_bvhCBuf.triIdx[--i].x, m_bvhCBuf.triIdx[j--].x);
		}

		// abort split if one of the sides is empty
		INT leftCnt{ i - node.leftFirstCntParent.y };
		if (leftCnt == 0 || leftCnt == node.leftFirstCntParent.z) {
			++leafs;
			updDepths(nodeId);
			return;
		}

		// create child nodes
		int leftIdx{ nodesUsed++ };
		m_bvhCBuf.bvhNode[leftIdx].leftFirstCntParent = {
			-1, node.leftFirstCntParent.y, leftCnt, nodeId
		};
		updNodeBounds(leftIdx);

		int rightIdx{ nodesUsed++ };
		m_bvhCBuf.bvhNode[rightIdx].leftFirstCntParent = {
			-1, i, node.leftFirstCntParent.z - leftCnt, nodeId
		};
		updNodeBounds(rightIdx);

		node.leftFirstCntParent = {
			leftIdx, -1, 0, node.leftFirstCntParent.w
		};

		// recurse
		Subdivide(leftIdx);
		Subdivide(rightIdx);
	}
};