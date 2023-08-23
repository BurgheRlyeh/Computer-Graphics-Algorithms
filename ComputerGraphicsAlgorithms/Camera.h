#pragma once

#include "framework.h"

struct Camera {
	DirectX::XMFLOAT3 poi{};
	float r{};
	float angZ{};
	float angY{};

	float rotationSpeed{ DirectX::XM_2PI };

	float forwardDelta{};
	float rightDelta{};

	void move(float delta);

	DirectX::XMFLOAT3 getDir();
	DirectX::XMFLOAT3 getUp();
	DirectX::XMFLOAT3 getForward();
	DirectX::XMFLOAT3 getRight();

	void getDirections(DirectX::XMFLOAT3& forward, DirectX::XMFLOAT3& right);
	DirectX::XMFLOAT3 getPosition();
};