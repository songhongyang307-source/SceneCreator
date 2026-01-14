

#include "shader_shadow.h"
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
#include "shadow_map.h"
#include "model.h"
#include "scene_model.h"
using namespace DirectX;


static ShadowMapRT g_sm;

static ID3D11SamplerState* g_shadowCmpSampler = nullptr;
static ID3D11Buffer* g_shadowCB = nullptr;

static bool CreateShadowCmpSampler(ID3D11Device* dev);
static bool CreateShadowCB(ID3D11Device* dev)
{
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(ShadowCB);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &g_shadowCB));
}

bool ShadowMap_Create(ID3D11Device* dev, ShadowMapRT& sm)
{
    ID3D11Texture2D* tex = nullptr;

    D3D11_TEXTURE2D_DESC td{};
    td.Width = sm.w; td.Height = sm.h;
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(dev->CreateTexture2D(&td, nullptr, &tex))) return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
    dsvd.Format = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(dev->CreateDepthStencilView(tex, &dsvd, &sm.dsv))) { tex->Release(); return false; }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
    srvd.Format = DXGI_FORMAT_R32_FLOAT;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(dev->CreateShaderResourceView(tex, &srvd, &sm.srv))) { tex->Release(); return false; }

    tex->Release();
    return true;
}

bool ShadowSystem_Initialize(ID3D11Device* dev)
{
    if (!dev) return false;

    if (!ShadowMap_Create(dev, g_sm)) return false;
    if (!CreateShadowCmpSampler(dev)) return false;
    if (!CreateShadowCB(dev)) return false;

    return true;
}

void ShadowSystem_Finalize()
{
    if (g_shadowCB) { g_shadowCB->Release(); g_shadowCB = nullptr; }
    if (g_shadowCmpSampler) { g_shadowCmpSampler->Release(); g_shadowCmpSampler = nullptr; }
    if (g_sm.srv) { g_sm.srv->Release(); g_sm.srv = nullptr; }
    if (g_sm.dsv) { g_sm.dsv->Release(); g_sm.dsv = nullptr; }
}

void ShadowSystem_RenderShadowMap(ID3D11DeviceContext* ctx, const DirectX::XMMATRIX& lightVP)
{
    if (!ctx || !g_sm.dsv) return;

    // 解绑 SRV，避免读写冲突
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    ctx->PSSetShaderResources(SHADOW_T_SLOT, 1, nullSRV);

    // 保存 backbuffer RT/DS
    ID3D11RenderTargetView* oldRTV = nullptr;
    ID3D11DepthStencilView* oldDSV = nullptr;
    ctx->OMGetRenderTargets(1, &oldRTV, &oldDSV);

    D3D11_VIEWPORT oldVP{};
    UINT oldVPCount = 1;
    ctx->RSGetViewports(&oldVPCount, &oldVP);

    // shadow DSV
    ctx->OMSetRenderTargets(0, nullptr, g_sm.dsv);


    // shadow viewport
    D3D11_VIEWPORT vp{};
    vp.Width = (float)g_sm.w;
    vp.Height = (float)g_sm.h;
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);

    // clear depth
    ctx->ClearDepthStencilView(g_sm.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // depth-only shadow shader（你要确保 shader_shadow.cpp 已改成 PS=nullptr 写深 ON + bias ON）
    ShadowShader_Begin();

    // 投影画
    SceneModel_DrawShadow(lightVP);

    ShadowShader_End();

    // 恢复 backbuffer RT/DS
    ctx->OMSetRenderTargets(1, &oldRTV, oldDSV);
    if (oldRTV) oldRTV->Release();
    if (oldDSV) oldDSV->Release();

    // 恢复主 viewport
    if (oldVPCount == 1)
        ctx->RSSetViewports(1, &oldVP);
}

void ShadowSystem_BindForMainPass(ID3D11DeviceContext* ctx, const DirectX::XMMATRIX& lightVP)
{
    if (!ctx || !g_sm.srv || !g_shadowCmpSampler || !g_shadowCB) return;

    ShadowCB cb{};
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(lightVP));
    cb.mtxLightVP = t;
    cb.shadowMapSize = XMFLOAT2((float)g_sm.w, (float)g_sm.h);
    cb.shadowBias = 0.0008f;
    cb.normalBias = 0.0020f;

    ctx->UpdateSubresource(g_shadowCB, 0, nullptr, &cb, 0, 0);
    ctx->PSSetConstantBuffers(SHADOW_B_SLOT, 1, &g_shadowCB);

    ctx->PSSetShaderResources(SHADOW_T_SLOT, 1, &g_sm.srv);
    ctx->PSSetSamplers(SHADOW_S_SLOT, 1, &g_shadowCmpSampler);
}

ID3D11ShaderResourceView* ShadowSystem_GetSRV() { return g_sm.srv; }



static bool CreateShadowCmpSampler(ID3D11Device* dev)
{

    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 1.0f;
    sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

    return SUCCEEDED(dev->CreateSamplerState(&sd, &g_shadowCmpSampler));
}
