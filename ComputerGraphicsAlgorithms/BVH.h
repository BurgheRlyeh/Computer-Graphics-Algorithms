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
		UINT left;
		UINT firstIdx, cnt;
	};

	struct BVHConstBuffer {
		Vector4 bbMin, bbMax;
		XMUINT4 leftFirstCnt;
	};

	static const UINT N{ 600 };

	Tri tri[N]{};
	UINT triIdx[N]{};

	BVHNode bvhNode[2 * N - 1]{};
	UINT rootNodeIdx{};
	UINT nodesUsed{ 1 };

	Cube* cube{};

	UINT cnt{};

public:
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
			triIdx[i] = i;
		}
		BVHNode& root = bvhNode[rootNodeIdx];
		root.left = 0;
		root.firstIdx = 0;
		root.cnt = 12 * cnt;
		updNodeBounds(rootNodeIdx);
		// subdivide recursively
		Subdivide(rootNodeIdx);
	}

private:
	void updNodeBounds(UINT nodeIdx) {
		BVHNode& node = bvhNode[nodeIdx];
		node.bbMin = { 1e30f, 1e30f, 1e30f };
		node.bbMax = { -1e30f, -1e30f, -1e30f };
		UINT first{ node.firstIdx };
		for (UINT i{}; i < node.cnt; ++i) {
			Tri& leafTri = tri[triIdx[first + i]];
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
		BVHNode& node{ bvhNode[nodeIdx] };
		if (node.cnt <= 2)
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
		UINT i{ node.firstIdx };
		UINT j{ i + node.cnt - 1 };
		while (i <= j) {
			//if (splitPos <= vecCompByIdx(tri[triIdx[i++]].center, axis))
			//	std::swap(triIdx[--i], triIdx[j--]);

			if (vecCompByIdx(tri[triIdx[i]].center, axis) < splitPos)
				++i;
			else
				std::swap(triIdx[i], triIdx[j--]);
		}

		// abort split if one of the sides is empty
		UINT leftCnt{ i - node.firstIdx };
		if (leftCnt == 0 || leftCnt == node.cnt)
			return;

		// create child nodes
		int leftIdx = nodesUsed++;
		bvhNode[leftIdx].firstIdx = node.firstIdx;
		bvhNode[leftIdx].cnt = leftCnt;

		int rightIdx = nodesUsed++;
		bvhNode[rightIdx].firstIdx = i;
		bvhNode[rightIdx].cnt = node.cnt - leftCnt;

		node.left = leftIdx;
		node.cnt = 0;
		updNodeBounds(leftIdx);
		updNodeBounds(rightIdx);

		// recurse
		Subdivide(leftIdx);
		Subdivide(rightIdx);
	}
};