#pragma once

#include <DirectXMath.h>
using namespace DirectX;

struct Ray
{
    XMFLOAT3 origin;
    XMFLOAT3 dir;
};

Ray ScreenPointToRay(float mouseX, float mouseY, float screenW, float screenH,
    const XMFLOAT4X4& view, const XMFLOAT4X4& proj);

bool RayHitPlaneY0(const Ray& ray, XMFLOAT3& hitPos);