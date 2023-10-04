#pragma once

#include "framework.h"
#include "Cube.h"

class Cube;

using namespace DirectX;
using namespace DirectX::SimpleMath;

class BVH {
	struct Tri {
		Vector4 v0{}, v1{}, v2{};
		Vector4 center{};
	};

	struct BVHNode {
		Vector4 bbMin, bbMax;
		XMUINT4 leftFirstCnt;
	};

	static const UINT N{ 300 };

	Tri tri[N]{};
	
	UINT rootNodeIdx{};
	UINT nodesUsed{ 1 };

	Cube* cube{};

	UINT cnt{};

public:
	struct BVHConstBuf {
		BVHNode bvhNode[2 * N - 1]{};
		XMUINT4 triIdx[N]{};
	} m_bvhCBuf;

	BVH(Cube* cube):
		cube(cube)
	{}

	void upd(UINT instCnt, Cube::ModelBuffer* modelBuffers) {
		cnt = instCnt;

		for (UINT m{}; m < instCnt; ++m) {
			for (UINT tr{}; tr < 12; ++tr) {
				XMINT4 trIds = cube->m_viBuffer.indices[tr];

				tri[12 * m + tr].v0 = Vector4::Transform(cube->m_viBuffer.vertices[trIds.x].point, modelBuffers[m].matrix);
				tri[12 * m + tr].v1 = Vector4::Transform(cube->m_viBuffer.vertices[trIds.y].point, modelBuffers[m].matrix);
				tri[12 * m + tr].v2 = Vector4::Transform(cube->m_viBuffer.vertices[trIds.z].point, modelBuffers[m].matrix);
				
			}
		}
	}

	void build() {
		for (int i{}; i < 12 * cnt; ++i) {
			tri[i].center = (tri[i].v0 + tri[i].v1 + tri[i].v2) / 3.f;
			m_bvhCBuf.triIdx[i].x = i;
		}
		BVHNode& root = m_bvhCBuf.bvhNode[rootNodeIdx];
		root.leftFirstCnt.x = 0;
		root.leftFirstCnt.y = 0;
		root.leftFirstCnt.z = 12 * cnt;
		updNodeBounds(rootNodeIdx);
		// subdivide recursively
		Subdivide(rootNodeIdx);
	}

private:
	void updNodeBounds(UINT nodeIdx) {
		BVHNode& node = m_bvhCBuf.bvhNode[nodeIdx];
		node.bbMin = { 1e30f, 1e30f, 1e30f };
		node.bbMax = { -1e30f, -1e30f, -1e30f };
		UINT first{ node.leftFirstCnt.y };
		for (UINT i{}; i < node.leftFirstCnt.z; ++i) {
			Tri& leafTri = tri[m_bvhCBuf.triIdx[first + i].x];
			node.bbMin = Vector4::Min(node.bbMin, leafTri.v0);
			node.bbMin = Vector4::Min(node.bbMin, leafTri.v1);
			node.bbMin = Vector4::Min(node.bbMin, leafTri.v2);
			node.bbMax = Vector4::Max(node.bbMax, leafTri.v0);
			node.bbMax = Vector4::Max(node.bbMax, leafTri.v1);
			node.bbMax = Vector4::Max(node.bbMax, leafTri.v2);
		}
	}

	float vecCompByIdx(Vector4 v, UINT idx) {
		switch (idx) {
		case 0: return v.x;
		case 1: return v.y;
		case 2: return v.z;
		case 3: return v.w;
		default: throw;
		}
	}

	void Subdivide(UINT nodeIdx) {
		// terminate recursion
		BVHNode& node{ m_bvhCBuf.bvhNode[nodeIdx] };
		if (node.leftFirstCnt.z <= 2)
			return;

		// determine split axis and position
		Vector4 extent{ node.bbMax - node.bbMin };

		//int a{ static_cast<int>(extent.x < extent.y) };
		//a += static_cast<int>(vecCompByIdx(extent, a) < extent.z);

		int axis{};
		if (extent.y > extent.x)
			axis = 1;
		if (extent.z > vecCompByIdx(extent, axis))
			axis = 2;

		float splitPos{ vecCompByIdx(node.bbMin, axis) + 0.5f * vecCompByIdx(extent, axis) };
		
		// in-place partition
		UINT i{ node.leftFirstCnt.y };
		UINT j{ i + node.leftFirstCnt.z - 1 };
		while (i <= j) {
			//if (splitPos <= vecCompByIdx(tri[triIdx[i++]].center, axis))
			//	std::swap(triIdx[--i], triIdx[j--]);

			if (vecCompByIdx(tri[m_bvhCBuf.triIdx[i].x].center, axis) < splitPos)
				++i;
			else
				std::swap(m_bvhCBuf.triIdx[i].x, m_bvhCBuf.triIdx[j--].x);
		}

		// abort split if one of the sides is empty
		UINT leftCnt{ i - node.leftFirstCnt.y };
		if (leftCnt == 0 || leftCnt == node.leftFirstCnt.z)
			return;

		// create child nodes
		int leftIdx = nodesUsed++;
		m_bvhCBuf.bvhNode[leftIdx].leftFirstCnt.y = node.leftFirstCnt.y;
		m_bvhCBuf.bvhNode[leftIdx].leftFirstCnt.z = leftCnt;

		int rightIdx = nodesUsed++;
		m_bvhCBuf.bvhNode[rightIdx].leftFirstCnt.y = i;
		m_bvhCBuf.bvhNode[rightIdx].leftFirstCnt.z = node.leftFirstCnt.z - leftCnt;

		node.leftFirstCnt.x = leftIdx;
		node.leftFirstCnt.z = 0;
		updNodeBounds(leftIdx);
		updNodeBounds(rightIdx);

		// recurse
		Subdivide(leftIdx);
		Subdivide(rightIdx);
	}
};