#include "picking.h"

Ray ScreenPointToRay(float mouseX, float mouseY, float screenW, float screenH, const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
{
    XMMATRIX V = XMLoadFloat4x4(&view);
    XMMATRIX P = XMLoadFloat4x4(&proj);
    XMMATRIX W = XMMatrixIdentity();   // 地形在世界空间里，先不考虑其它 world

    // 1) 屏幕坐标 -> 世界空间的 “近裁剪面点” 和 “远裁剪面点”
    XMVECTOR nearPoint = XMVector3Unproject(
        XMVectorSet(mouseX, mouseY, 0.0f, 1.0f),   // z = 0 → near
        0.0f, 0.0f, screenW, screenH,
        0.0f, 1.0f,   // 深度 0~1
        P, V, W
    );

    XMVECTOR farPoint = XMVector3Unproject(
        XMVectorSet(mouseX, mouseY, 1.0f, 1.0f),   // z = 1 → far
        0.0f, 0.0f, screenW, screenH,
        0.0f, 1.0f,
        P, V, W);


    // 2) 射线：原点是 nearPoint，方向是 far- near
    XMVECTOR dir = XMVector3Normalize(farPoint - nearPoint);

    Ray r;
    XMStoreFloat3(&r.origin, nearPoint);
    XMStoreFloat3(&r.dir, dir);
    return r;
}

bool RayHitPlaneY0(const Ray& ray, XMFLOAT3& hitPos)
{
    float oy = ray.origin.y;
    float dy = ray.dir.y;

    if (fabs(dy) < 1e-5f) return false;

    float t = -oy / dy;
    if (t < 0.0f) return false;

    XMVECTOR o = XMLoadFloat3(&ray.origin);
    XMVECTOR d = XMLoadFloat3(&ray.dir);
    XMVECTOR p = o + d * t;

    XMStoreFloat3(&hitPos, p);
    return true;

}
