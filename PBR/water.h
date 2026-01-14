#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "picking.h" // 你的 Ray 结构

struct WaterTransform
{
    DirectX::XMFLOAT3 position{ 0,0,0 };
    DirectX::XMFLOAT3 rotation{ 0,0,0 };
    DirectX::XMFLOAT3 scale{ 1,1,1 };
};
WaterTransform* Water_GetTransformPtr();
DirectX::XMMATRIX Water_GetWorldMatrix();
void Water_ApplyTransformNow();              // 立刻更新 g_world（避免等到下一帧）
bool Water_Pick(const Ray& ray, DirectX::XMFLOAT3* outHitW = nullptr);


bool Water_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx);
void Water_Finalize();
void Water_Update(float dt);
void Water_Draw(const XMMATRIX& view, const XMMATRIX& proj, const XMFLOAT3& eyePosW);

ID3D11DepthStencilView* Direct3D_GetMainDSV_ReadOnly();