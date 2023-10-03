#pragma once

#include "framework.h"
#include "Cube.h"

class Cube;

using namespace DirectX;
using namespace DirectX::SimpleMath;

struct Tri {
	Vector4 v0{}, v1{}, v2{};
	Vector4 center{};
};

struct BVHNode {
	Vector4 aabbMin, aabbMax;
	UINT left;
	UINT firstTriIdx, triCnt;
};

class BVH {
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
		root.firstTriIdx = 0;
		root.triCnt = 12 * cnt;
		updNodeBounds(rootNodeIdx);
		// subdivide recursively
		Subdivide(rootNodeIdx);
	}

private:
	void updNodeBounds(UINT nodeIdx) {
		BVHNode& node = bvhNode[nodeIdx];
		node.aabbMin = { 1e30f, 1e30f, 1e30f };
		node.aabbMax = { -1e30f, -1e30f, -1e30f };
		UINT first{ node.firstTriIdx };
		for (UINT i{}; i < node.triCnt; ++i) {
			Tri& leafTri = tri[triIdx[first + i]];
			node.aabbMin = Vector4::Min(node.aabbMin, leafTri.v0);
			node.aabbMin = Vector4::Min(node.aabbMin, leafTri.v1);
			node.aabbMin = Vector4::Min(node.aabbMin, leafTri.v2);
			node.aabbMax = Vector4::Max(node.aabbMax, leafTri.v0);
			node.aabbMax = Vector4::Max(node.aabbMax, leafTri.v1);
			node.aabbMax = Vector4::Max(node.aabbMax, leafTri.v2);
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
		if (node.triCnt <= 2)
			return;

		// determine split axis and position
		Vector4 extent{ node.aabbMax - node.aabbMin };

		//int a{ static_cast<int>(extent.x < extent.y) };
		//a += static_cast<int>(vecCompByIdx(extent, a) < extent.z);

		int axis{};
		if (extent.y > extent.x)
			axis = 1;
		if (extent.z > vecCompByIdx(extent, axis))
			axis = 2;

		float splitPos{ vecCompByIdx(node.aabbMin, axis) + 0.5f * vecCompByIdx(extent, axis) };
		
		// in-place partition
		UINT i{ node.firstTriIdx };
		UINT j{ i + node.triCnt - 1 };
		while (i <= j) {
			//if (splitPos <= vecCompByIdx(tri[triIdx[i++]].center, axis))
			//	std::swap(triIdx[--i], triIdx[j--]);

			if (vecCompByIdx(tri[triIdx[i]].center, axis) < splitPos)
				++i;
			else
				std::swap(triIdx[i], triIdx[j--]);
		}

		// abort split if one of the sides is empty
		UINT leftCnt{ i - node.firstTriIdx };
		if (leftCnt == 0 || leftCnt == node.triCnt)
			return;

		// create child nodes
		int leftIdx = nodesUsed++;
		bvhNode[leftIdx].firstTriIdx = node.firstTriIdx;
		bvhNode[leftIdx].triCnt = leftCnt;

		int rightIdx = nodesUsed++;
		bvhNode[rightIdx].firstTriIdx = i;
		bvhNode[rightIdx].triCnt = node.triCnt - leftCnt;

		node.left = leftIdx;
		node.triCnt = 0;
		updNodeBounds(leftIdx);
		updNodeBounds(rightIdx);

		// recurse
		Subdivide(leftIdx);
		Subdivide(rightIdx);
	}
};