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

	void upd(INT instCnt, Vector4(&vertices)[24], XMINT4(&indices)[12], Matrix(&modelMatrices)[25]) {
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

	float splitByStepSAH(BVHNode& node, int& axis, float& splitPos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			float bmin = vecCompByIdx(node.bb.bmin, a);
			float bmax = vecCompByIdx(node.bb.bmax, a);

			if (bmin == bmax)
				continue;

			float scale = (bmax - bmin) / sahStep;
			for (int i{ 1 }; i < sahStep; ++i) {
				float candidatePos{ bmin + i * scale };
				float cost = evaluateSAH(node, a, candidatePos);
				if (cost < bestCost) {
					axis = a;
					splitPos = candidatePos;
					bestCost = cost;
				}
			}
		}

		return bestCost;
	}

	struct Bin {
		AABB bounds{};
		int triCnt{};
	};

	float splitByStepSAHWithBins(BVHNode& node, int& axis, float& splitPos) {
		float bestCost{ std::numeric_limits<float>::max() };
		for (int a{}; a < 3; ++a) {
			float boundsMin{ std::numeric_limits<float>::max() },
				boundsMax{ std::numeric_limits<float>::min() };
			for (int i{}; i < node.leftFirstCntParent.z; ++i) {
				Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];
				boundsMin = min(boundsMin, vecCompByIdx(t.center, a));
				boundsMax = max(boundsMax, vecCompByIdx(t.center, a));
			}
			if (boundsMin == boundsMax)
				continue;
			Bin bin[8]{};
			float scale = sahStep / (boundsMax - boundsMin);
			for (int i{}; i < node.leftFirstCntParent.z; ++i) {
				Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];
				int binIdx = min(sahStep - 1, (int)((vecCompByIdx(t.center, a) - boundsMin) * scale));
				bin[binIdx].triCnt++;
				bin[binIdx].bounds.grow(t.v0);
				bin[binIdx].bounds.grow(t.v1);
				bin[binIdx].bounds.grow(t.v2);
			}
			float leftArea[7], rightArea[7];
			int leftCnt[7], rightCnt[7];
			AABB leftBox{}, rightBox{};
			int leftSum{}, rightSum{};
			for (int i{}; i < sahStep - 1; ++i) {
				leftSum += bin[i].triCnt;
				leftCnt[i] = leftSum;
				leftBox.grow(bin[i].bounds);
				leftArea[i] = leftBox.area();

				rightSum += bin[sahStep - 1 - i].triCnt;
				rightCnt[sahStep - 2 - i] = rightSum;
				rightBox.grow(bin[sahStep - 1 - i].bounds);
				rightArea[sahStep - 2 - i] = rightBox.area();
			}
			scale = (boundsMax - boundsMin) / sahStep;
			for (int i{}; i < sahStep - 1; ++i) {
				float planeCost{ leftCnt[i] * leftArea[i] + rightCnt[i] * rightArea[i] };
				if (planeCost < bestCost) {
					axis = a;
					splitPos = boundsMin + scale * (i + 1);
					bestCost = planeCost;
				}
			}
		}
		return bestCost;
	}

	//float splitByStepSAHWithBins(BVHNode& node, int& axis, float& splitPos) {
	//	float bestCost{ std::numeric_limits<float>::max() };
	//	for (int a{}; a < 3; ++a) {
	//		//float bmin = vecCompByIdx(node.bb.bmin, a);
	//		//float bmax = vecCompByIdx(node.bb.bmax, a);

	//		float boundsMin = std::numeric_limits<float>::max(), boundsMax = std::numeric_limits<float>::min();
	//		for (int i{}; i < node.leftFirstCntParent.z; ++i) {
	//			Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];
	//			boundsMin = min(boundsMin, vecCompByIdx(t.center, a));
	//			boundsMax = max(boundsMax, vecCompByIdx(t.center, a));
	//		}

	//		if (boundsMin == boundsMax)
	//			continue;

	//		float scale = sahStep / (boundsMax - boundsMin);
	//		for (int i{}; i < node.leftFirstCntParent.z; ++i) {
	//			Tri& t = tri[m_bvhCBuf.triIdx[node.leftFirstCntParent.y + i].x];
	//			int binId = min(sahStep - 1, (int)((vecCompByIdx(t.center, a) - boundsMin) * scale));

	//			bins[binId].triCnt++;
	//			bins[binId].bb.grow(t.v0);
	//			bins[binId].bb.grow(t.v1);
	//			bins[binId].bb.grow(t.v2);
	//		}

	//		AABB leftBox{}, rightBox{};
	//		int leftSum{}, rightSum{};
	//		for (int i{}; i < sahStep - 1; ++i) {
	//			leftSum += bins[i].triCnt;
	//			lc[i] = leftSum;
	//			leftBox.grow(bins[i].bb);
	//			la[i] = leftBox.area();

	//			rightSum += bins[sahStep - 1 - i].triCnt;
	//			rc[sahStep - 2 - i] = rightSum;
	//			rightBox.grow(bins[sahStep - 1 - i].bb);
	//			ra[sahStep - 2 - i] = rightBox.area();
	//		}

	//		scale = (boundsMax - boundsMin) / sahStep;
	//		for (int i{}; i < sahStep - 1; ++i) {
	//			float cost = lc[i] * la[i] + rc[i] * ra[i];
	//			if (cost < bestCost) {
	//				axis = a;
	//				splitPos = boundsMin + scale * (i + 1);
	//				bestCost = cost;
	//			}
	//		}
	//	}

	//	return bestCost;
	//}

	void Subdivide(INT nodeId) {
		// terminate recursion
		BVHNode& node{ m_bvhCBuf.bvhNode[nodeId] };

		// determine split axis and position
		int axis{};
		float splitPos{};

		if (isSAH) {
			float cost;

			if (isStepSAH) {
				cost = isBinsSAH ? splitByStepSAHWithBins(node, axis, splitPos) : splitByStepSAH(node, axis, splitPos);
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
			//Vector4 center = (t.v0 + t.v1 + t.v2) / 3.f;
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