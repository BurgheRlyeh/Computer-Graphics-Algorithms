#pragma once

#include "framework.h"

#include "Cube.h"
#include "AABB.h"

class Cube;
struct AABB;

using namespace DirectX;
using namespace DirectX::SimpleMath;

class BVH {
	struct Tri {
		Vector4 v0{}, v1{}, v2{};
	};

	struct BVHNode {
		AABB bb;
		XMINT4 leftFirstCntParent;
	};

	static const INT N{ 300 };

	Tri tri[N]{};
	
	INT rootNodeIdx{};
	INT nodesUsed{ 1 };

	Cube* cube{};

	INT cnt{};

public:
	struct BVHConstBuf {
		BVHNode bvhNode[2 * N - 1]{};
		XMINT4 triIdx[N]{};
	} m_bvhCBuf;

	BVH(Cube* cube):
		cube(cube)
	{}

	//void upd(INT instCnt, Vector4* vertices, INT verticesCnt, XMINT4* indices, INT indicesCnt, Matrix* modelMatrices) {
	void upd(UINT instCnt, Cube::ModelBuffer* modelBuffers) {
		cnt = instCnt;

		for (INT m{}; m < instCnt; ++m) {
			Matrix matrix{ modelBuffers[m].matrix };

			for (INT tr{}; tr < 12; ++tr) {
				INT id{ 12 * m + tr };
				XMINT4 trIds{ cube->m_viBuffer.indices[tr] };

				tri[id].v0 = Vector4::Transform(cube->m_viBuffer.vertices[trIds.x].point, matrix);
				tri[id].v1 = Vector4::Transform(cube->m_viBuffer.vertices[trIds.y].point, matrix);
				tri[id].v2 = Vector4::Transform(cube->m_viBuffer.vertices[trIds.z].point, matrix);
			}
		}
	}

	void build() {
		for (int i{}; i < 12 * cnt; ++i) {
			m_bvhCBuf.triIdx[i].x = i;
		}
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
		node.bb.bmin = { 1e30f, 1e30f, 1e30f };
		node.bb.bmax = { -1e30f, -1e30f, -1e30f };
		INT first{ node.leftFirstCntParent.y };
		for (INT i{}; i < node.leftFirstCntParent.z; ++i) {
			Tri& leafTri = tri[m_bvhCBuf.triIdx[first + i].x];

			node.bb.bmin = Vector4::Min(node.bb.bmin, leafTri.v0);
			node.bb.bmin = Vector4::Min(node.bb.bmin, leafTri.v1);
			node.bb.bmin = Vector4::Min(node.bb.bmin, leafTri.v2);

			node.bb.bmax = Vector4::Max(node.bb.bmax, leafTri.v0);
			node.bb.bmax = Vector4::Max(node.bb.bmax, leafTri.v1);
			node.bb.bmax = Vector4::Max(node.bb.bmax, leafTri.v2);
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

	void Subdivide(INT nodeIdx) {
		// terminate recursion
		BVHNode& node{ m_bvhCBuf.bvhNode[nodeIdx] };
		if (node.leftFirstCntParent.z <= 2)
			return;

		// determine split axis and position
		Vector4 extent{ node.bb.bmax - node.bb.bmin };
		int axis{ static_cast<int>(extent.x < extent.y) };
		axis += static_cast<int>(vecCompByIdx(extent, axis) < extent.z);
		float splitPos{ vecCompByIdx(node.bb.bmin + extent / 2.f, axis) };
		
		// in-place partition
		INT i{ node.leftFirstCntParent.y };
		INT j{ i + node.leftFirstCntParent.z - 1 };
		while (i <= j) {
			Tri& t = tri[m_bvhCBuf.triIdx[i++].x];
			Vector4 center = (t.v0 + t.v1 + t.v2) / 3.f;

			if (splitPos <= vecCompByIdx(center, axis))
				std::swap(m_bvhCBuf.triIdx[--i].x, m_bvhCBuf.triIdx[j--].x);
		}

		// abort split if one of the sides is empty
		INT leftCnt{ i - node.leftFirstCntParent.y };
		if (leftCnt == 0 || leftCnt == node.leftFirstCntParent.z)
			return;

		// create child nodes
		int leftIdx{ nodesUsed++ };
		m_bvhCBuf.bvhNode[leftIdx].leftFirstCntParent = {
			-1, node.leftFirstCntParent.y, leftCnt, nodeIdx
		};
		updNodeBounds(leftIdx);

		int rightIdx{ nodesUsed++ };
		m_bvhCBuf.bvhNode[rightIdx].leftFirstCntParent = {
			-1, i, node.leftFirstCntParent.z - leftCnt, nodeIdx
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