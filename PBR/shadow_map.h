#pragma once

#include "direct3d.h"
#include "shader_shadow.h"


struct ShadowCB
{
    DirectX::XMFLOAT4X4 mtxLightVP;     // 注意上传前转置
    DirectX::XMFLOAT2   shadowMapSize;  // (w,h)
    float shadowBias;
    float normalBias;
};

struct ShadowMapRT
{
    int w = 2048, h = 2048;
    ID3D11DepthStencilView* dsv = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
};

static constexpr UINT SHADOW_T_SLOT = 20; // t20
static constexpr UINT SHADOW_S_SLOT = 1;  // s1
static constexpr UINT SHADOW_B_SLOT = 7;  // b7

bool ShadowMap_Create(ID3D11Device* dev, ShadowMapRT& sm);


bool ShadowSystem_Initialize(ID3D11Device* dev);
void ShadowSystem_Finalize();

// 这一帧：生成 shadow map（depth-only）
void ShadowSystem_RenderShadowMap(ID3D11DeviceContext* ctx, const DirectX::XMMATRIX& lightVP);

// 这一帧：主渲染用（绑定 t10/s10 + b6）
void ShadowSystem_BindForMainPass(ID3D11DeviceContext* ctx, const DirectX::XMMATRIX& lightVP);

// 给外部用：如需要手动绑定SRV
ID3D11ShaderResourceView* ShadowSystem_GetSRV();