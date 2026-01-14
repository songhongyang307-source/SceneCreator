#include "shader_lightshaft.h"
#include "direct3d.h"
#include <vector>
#include <fstream>
#include <Windows.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) do{ if(x){ (x)->Release(); (x)=nullptr; } }while(0)
#endif

using namespace DirectX;

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;

static ID3D11VertexShader* g_vs = nullptr;
static ID3D11PixelShader* g_psMask = nullptr;
static ID3D11PixelShader* g_psMaskMSAA = nullptr;
static ID3D11PixelShader* g_psBlur = nullptr;
static ID3D11PixelShader* g_psComp = nullptr;

static ID3D11Buffer* g_cbShaft = nullptr;    // b7
static ID3D11SamplerState* g_sClamp = nullptr;     // s0

static ID3D11Texture2D* g_maskTex = nullptr;
static ID3D11RenderTargetView* g_maskRTV = nullptr;
static ID3D11ShaderResourceView* g_maskSRV = nullptr;

static ID3D11Texture2D* g_raysTex = nullptr;
static ID3D11RenderTargetView* g_raysRTV = nullptr;
static ID3D11ShaderResourceView* g_raysSRV = nullptr;

static ID3D11ShaderResourceView* g_depthSRV = nullptr; // t41
static bool g_depthIsMSAA = false;

static int g_ds = 2; // downsample
static UINT g_rtW = 0, g_rtH = 0;

struct CB_Shaft
{
    XMFLOAT4X4 invProjT;   // transpose(invProj)
    XMFLOAT2   sunUV;
    float      intensity;
    float      skyDepthEps;

    XMFLOAT3   sunDirV;
    float      sunThreshold;

    float      sunPower;
    float      density;
    float      decay;
    float      weight;

    float      exposure;
    XMFLOAT2   invTargetSize;
    float      pad0;
};

static bool LoadCSO(const char* path, std::vector<unsigned char>& out)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    ifs.seekg(0, std::ios::end);
    size_t sz = (size_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    out.resize(sz);
    ifs.read((char*)out.data(), sz);
    return true;
}

static bool CreateRT(UINT w, UINT h, ID3D11Texture2D** tex, ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv)
{
    SAFE_RELEASE(*srv);
    SAFE_RELEASE(*rtv);
    SAFE_RELEASE(*tex);

    D3D11_TEXTURE2D_DESC td{};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R16_FLOAT;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(g_dev->CreateTexture2D(&td, nullptr, tex))) return false;
    if (FAILED(g_dev->CreateRenderTargetView(*tex, nullptr, rtv))) return false;
    if (FAILED(g_dev->CreateShaderResourceView(*tex, nullptr, srv))) return false;
    return true;
}

static bool EnsureTargets(int downsample)
{
    downsample = (downsample <= 1) ? 1 : downsample;
    if (downsample != 1 && downsample != 2 && downsample != 4) downsample = 2;

    UINT fullW = Direct3D_GetBackBufferWidth();
    UINT fullH = Direct3D_GetBackBufferHeight();
    if (fullW == 0) fullW = 1;
    if (fullH == 0) fullH = 1;

    UINT w = (fullW + downsample - 1) / downsample;
    UINT h = (fullH + downsample - 1) / downsample;

    if ((int)downsample == g_ds && w == g_rtW && h == g_rtH && g_maskTex && g_raysTex)
        return true;

    g_ds = downsample;
    g_rtW = w;
    g_rtH = h;

    if (!CreateRT(w, h, &g_maskTex, &g_maskRTV, &g_maskSRV)) return false;
    if (!CreateRT(w, h, &g_raysTex, &g_raysRTV, &g_raysSRV)) return false;
    return true;
}

namespace ShaderLightShaft
{
    bool Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    {
        if (!dev || !ctx) return false;
        g_dev = dev;
        g_ctx = ctx;

        // VS
        {
            std::vector<unsigned char> bin;
            if (!LoadCSO("shader_vertex_lightshaft.cso", bin)) return false;
            if (FAILED(g_dev->CreateVertexShader(bin.data(), bin.size(), nullptr, &g_vs))) return false;
        }

        // PS mask
        {
            std::vector<unsigned char> bin;
            if (!LoadCSO("shader_pixel_lightshaft_mask.cso", bin)) return false;
            if (FAILED(g_dev->CreatePixelShader(bin.data(), bin.size(), nullptr, &g_psMask))) return false;
        }

        // PS mask msaa
        {
            std::vector<unsigned char> bin;
            if (!LoadCSO("shader_pixel_lightshaft_mask_msaa.cso", bin)) return false;
            if (FAILED(g_dev->CreatePixelShader(bin.data(), bin.size(), nullptr, &g_psMaskMSAA))) return false;
        }

        // PS blur
        {
            std::vector<unsigned char> bin;
            if (!LoadCSO("shader_pixel_lightshaft_blur.cso", bin)) return false;
            if (FAILED(g_dev->CreatePixelShader(bin.data(), bin.size(), nullptr, &g_psBlur))) return false;
        }

        // PS composite
        {
            std::vector<unsigned char> bin;
            if (!LoadCSO("shader_pixel_lightshaft_comp.cso", bin)) return false;
            if (FAILED(g_dev->CreatePixelShader(bin.data(), bin.size(), nullptr, &g_psComp))) return false;
        }

        // CB b7
        {
            D3D11_BUFFER_DESC bd{};
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bd.ByteWidth = (UINT)((sizeof(CB_Shaft) + 15) & ~15);
            if (FAILED(g_dev->CreateBuffer(&bd, nullptr, &g_cbShaft))) return false;
        }

        // sampler s0 clamp
        {
            D3D11_SAMPLER_DESC sd{};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            sd.MaxLOD = D3D11_FLOAT32_MAX;
            if (FAILED(g_dev->CreateSamplerState(&sd, &g_sClamp))) return false;
        }

        // default targets
        if (!EnsureTargets(2)) return false;

        return true;
    }

    void Finalize()
    {
        SAFE_RELEASE(g_raysSRV);
        SAFE_RELEASE(g_raysRTV);
        SAFE_RELEASE(g_raysTex);

        SAFE_RELEASE(g_maskSRV);
        SAFE_RELEASE(g_maskRTV);
        SAFE_RELEASE(g_maskTex);

        SAFE_RELEASE(g_sClamp);
        SAFE_RELEASE(g_cbShaft);

        SAFE_RELEASE(g_psComp);
        SAFE_RELEASE(g_psBlur);
        SAFE_RELEASE(g_psMaskMSAA);
        SAFE_RELEASE(g_psMask);
        SAFE_RELEASE(g_vs);

        g_depthSRV = nullptr;
        g_depthIsMSAA = false;

        g_dev = nullptr;
        g_ctx = nullptr;
    }

    void SetSceneDepthSRV(ID3D11ShaderResourceView* depthSRV, bool depthIsMSAA)
    {
        g_depthSRV = depthSRV;
        g_depthIsMSAA = depthIsMSAA;
    }

    void Draw(const XMMATRIX& view, const XMMATRIX& proj, const LightShaftParams& p)
    {
        if (!g_ctx || !g_vs || !g_psMask || !g_psBlur || !g_psComp || !g_cbShaft) return;
        if (!g_depthSRV) return;
        if (!EnsureTargets(p.downsample)) return;

        // ---- 计算 sunDirV & sunUV ----
        // p.lightDirW 是“光线传播方向”，太阳方向 = -lightDir
        XMVECTOR Lw = XMVector3Normalize(XMLoadFloat3(&p.lightDirW));
        XMVECTOR Sw = XMVectorNegate(Lw); // toward sun
        XMVECTOR Sv = XMVector3TransformNormal(Sw, view);
        Sv = XMVector3Normalize(Sv);

        XMFLOAT3 sunDirV{};
        XMStoreFloat3(&sunDirV, Sv);

        // 太阳在背后就不画（很重要，否则 uv 会乱飘）
        if (sunDirV.z <= 0.0f) return;

        // 用一个很远的 view-space 点投影到屏幕求 sunUV
        XMVECTOR sunPosV = Sv * 1000.0f;
        XMVECTOR clip = XMVector4Transform(XMVectorSetW(sunPosV, 1.0f), proj);

        XMFLOAT4 c{};
        XMStoreFloat4(&c, clip);
        if (fabsf(c.w) < 1e-6f) return;

        float ndcX = c.x / c.w;
        float ndcY = c.y / c.w;

        XMFLOAT2 sunUV{};
        sunUV.x = ndcX * 0.5f + 0.5f;
        sunUV.y = 1.0f - (ndcY * 0.5f + 0.5f); // y 翻转，匹配你 fullscreen uv

        // ---- 填 CB ----
        CB_Shaft cb{};
        XMMATRIX invP = XMMatrixInverse(nullptr, proj);
        XMStoreFloat4x4(&cb.invProjT, XMMatrixTranspose(invP));

        cb.sunUV = sunUV;
        cb.intensity = p.intensity;
        cb.skyDepthEps = p.skyDepthEps;

        cb.sunDirV = sunDirV;
        cb.sunThreshold = p.sunThreshold;

        cb.sunPower = p.sunPower;
        cb.density = p.density;
        cb.decay = p.decay;
        cb.weight = p.weight;

        cb.exposure = p.exposure;
        cb.invTargetSize = XMFLOAT2(1.0f / (float)g_rtW, 1.0f / (float)g_rtH);
        cb.pad0 = 0.0f;

        // ---- 保存状态 ----
        ID3D11RenderTargetView* oldRTV = nullptr;
        ID3D11DepthStencilView* oldDSV = nullptr;
        g_ctx->OMGetRenderTargets(1, &oldRTV, &oldDSV);

        ID3D11BlendState* oldBS = nullptr;
        ID3D11DepthStencilState* oldDSS = nullptr;
        ID3D11RasterizerState* oldRS = nullptr;
        float oldBF[4]{};
        UINT oldMask = 0, oldRef = 0;
        g_ctx->OMGetBlendState(&oldBS, oldBF, &oldMask);
        g_ctx->OMGetDepthStencilState(&oldDSS, &oldRef);
        g_ctx->RSGetState(&oldRS);

        D3D11_VIEWPORT oldVP{};
        UINT vpCount = 1;
        g_ctx->RSGetViewports(&vpCount, &oldVP);

        // ---- 公共绑定（全屏三角）----
        g_ctx->VSSetShader(g_vs, nullptr, 0);
        g_ctx->IASetInputLayout(nullptr);
        g_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        g_ctx->PSSetSamplers(0, 1, &g_sClamp);
        g_ctx->PSSetConstantBuffers(7, 1, &g_cbShaft);
        g_ctx->UpdateSubresource(g_cbShaft, 0, nullptr, &cb, 0, 0);

        // ============= PASS 1: Mask（写入 g_maskRTV，读 depth t41）=============
        {
            // 低分辨率 viewport
            D3D11_VIEWPORT vp{};
            vp.TopLeftX = 0; vp.TopLeftY = 0;
            vp.Width = (FLOAT)g_rtW;
            vp.Height = (FLOAT)g_rtH;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            g_ctx->RSSetViewports(1, &vp);

            // 不绑 DSV（避免 depth SRV/DSV hazard）
            g_ctx->OMSetRenderTargets(1, &g_maskRTV, nullptr);

            float clear0[4] = { 0,0,0,0 };
            g_ctx->ClearRenderTargetView(g_maskRTV, clear0);

            g_ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF); // 覆盖写
            Direct3D_SetDepthEnable(false);

            g_ctx->PSSetShader(g_depthIsMSAA ? g_psMaskMSAA : g_psMask, nullptr, 0);
            g_ctx->PSSetShaderResources(41, 1, &g_depthSRV);

            g_ctx->Draw(3, 0);

            ID3D11ShaderResourceView* nullSRV = nullptr;
            g_ctx->PSSetShaderResources(41, 1, &nullSRV);
        }

        // ============= PASS 2: Blur（写入 g_raysRTV，读 mask t50）=============
        {
            g_ctx->OMSetRenderTargets(1, &g_raysRTV, nullptr);

            float clear0[4] = { 0,0,0,0 };
            g_ctx->ClearRenderTargetView(g_raysRTV, clear0);

            g_ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            Direct3D_SetDepthEnable(false);

            g_ctx->PSSetShader(g_psBlur, nullptr, 0);
            g_ctx->PSSetShaderResources(50, 1, &g_maskSRV);

            g_ctx->Draw(3, 0);

            ID3D11ShaderResourceView* nullSRV = nullptr;
            g_ctx->PSSetShaderResources(50, 1, &nullSRV);
        }

        // ============= PASS 3: Composite（写回主屏幕，加算）=============
        {
            // 恢复全分辨率 viewport
            g_ctx->RSSetViewports(1, &oldVP);

            // 绑回主 RT（用只读DSV更安全；没有也行）
            ID3D11RenderTargetView* mainRTV = Direct3D_GetMainRTV();
            ID3D11DepthStencilView* dsvRO = Direct3D_GetMainDSV_ReadOnly();
            g_ctx->OMSetRenderTargets(1, &mainRTV, dsvRO);

            Direct3D_SetAlphaBlendAdd();
            Direct3D_SetDepthEnable(false);

            g_ctx->PSSetShader(g_psComp, nullptr, 0);
            g_ctx->PSSetShaderResources(51, 1, &g_raysSRV);

            g_ctx->Draw(3, 0);

            ID3D11ShaderResourceView* nullSRV = nullptr;
            g_ctx->PSSetShaderResources(51, 1, &nullSRV);
        }

        // ---- 恢复状态 ----
        g_ctx->RSSetState(oldRS);
        g_ctx->OMSetBlendState(oldBS, oldBF, oldMask);
        g_ctx->OMSetDepthStencilState(oldDSS, oldRef);
        g_ctx->OMSetRenderTargets(1, &oldRTV, oldDSV);
        g_ctx->RSSetViewports(1, &oldVP);

        SAFE_RELEASE(oldRTV);
        SAFE_RELEASE(oldDSV);
        SAFE_RELEASE(oldBS);
        SAFE_RELEASE(oldDSS);
        SAFE_RELEASE(oldRS);
    }
}
