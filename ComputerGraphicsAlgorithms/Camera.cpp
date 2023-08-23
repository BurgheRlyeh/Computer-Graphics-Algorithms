#include "Camera.h"

using namespace DirectX;

void Camera::move(float delta) {
    DirectX::XMFLOAT3 cf, cr;
    getDirections(cf, cr);
    poi = {
        poi.x + delta * (cf.x * forwardDelta + cr.x * rightDelta),
        poi.y + delta * (cf.y * forwardDelta + cr.y * rightDelta),
        poi.z + delta * (cf.z * forwardDelta + cr.z * rightDelta)
    };
}

DirectX::XMFLOAT3 Camera::getDir() {
    return {
        cosf(angY) * cosf(angZ),
        sinf(angY),
        cosf(angY) * sinf(angZ)
    };
}

DirectX::XMFLOAT3 Camera::getUp() {
    return {
        cosf(angY + DirectX::XM_PIDIV2) * cosf(angZ),
        sinf(angY + DirectX::XM_PIDIV2),
        cosf(angY + DirectX::XM_PIDIV2) * sinf(angZ)
    };
}

void Camera::getDirections(DirectX::XMFLOAT3& forward, DirectX::XMFLOAT3& right) {
    auto dir{ getDir() };
    auto up{ getUp() };

    auto dirVector{ DirectX::XMLoadFloat3(&dir) };
    auto upVector{ DirectX::XMLoadFloat3(&up) };

    auto forwardVector{
        DirectX::XMMax(
            fabs(DirectX::XMVectorGetX(dirVector)),
            fabs(DirectX::XMVectorGetZ(dirVector))
        ) <= 1e-5f ? upVector : dirVector
    };
    DirectX::XMVectorSetY(forwardVector, 0.0f);
    DirectX::XMVector3Normalize(forwardVector);
    DirectX::XMStoreFloat3(&forward, forwardVector);


    auto rightVector{ DirectX::XMVector3Cross(upVector, dirVector) };
    DirectX::XMVectorSetY(rightVector, 0.0f);
    DirectX::XMVector3Normalize(rightVector);
    DirectX::XMStoreFloat3(&right, rightVector);
}

DirectX::XMFLOAT3 Camera::getForward() {
    auto dir{ getDir() };
    auto up{ getUp() };

    auto dirVector{ DirectX::XMLoadFloat3(&dir) };
    auto upVector{ DirectX::XMLoadFloat3(&up) };

    auto forwardVector{
        DirectX::XMMax(
            fabs(DirectX::XMVectorGetX(dirVector)),
            fabs(DirectX::XMVectorGetZ(dirVector))
        ) <= 1e-5f ? upVector : dirVector
    };
    DirectX::XMVectorSetY(forwardVector, 0.0f);
    DirectX::XMVector3Normalize(forwardVector);

    DirectX::XMFLOAT3 forward{};
    DirectX::XMStoreFloat3(&forward, forwardVector);

    return forward;
}

DirectX::XMFLOAT3 Camera::getRight() {
    auto dir{ getDir() };
    auto up{ getUp() };

    auto dirVector{ DirectX::XMLoadFloat3(&dir) };
    auto upVector{ DirectX::XMLoadFloat3(&up) };

    auto rightVector{ DirectX::XMVector3Cross(upVector, dirVector) };
    //DirectX::XMVectorScale(rightVector, -1.0f);

    DirectX::XMFLOAT3 right{};
    DirectX::XMStoreFloat3(&right, rightVector);

    //return right;
    return {
        - right.x,
        - right.y,
        - right.z
    };
}

DirectX::XMFLOAT3 Camera::getPosition() {
    auto dir{ getDir() };
    return {
        poi.x + r * dir.x,
        poi.y + r * dir.y,
        poi.z + r * dir.z
    };
}
