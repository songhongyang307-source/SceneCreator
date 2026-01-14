#include "shader_volumetric_fog.h"
#include "direct3d.h"
#include <fstream>
#include <vector>
#include <Windows.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) do{ if(x){ (x)->Release(); (x)=nullptr; } }while(0)
#endif

using namespace DirectX;

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;

static ID3D11VertexShader* g_vs = nullptr;
static ID3D11PixelShader* g_ps = nullptr;
static ID3D11PixelShader* g_psMSAA = nullptr;

static ID3D11Buffer* g_cbFog = nullptr;        // b6
static ID3D11SamplerState* g_sClamp = nullptr; // s0

static ID3D11ShaderResourceView* g_sceneColorSRV = nullptr; // t40
static ID3D11ShaderResourceView* g_sceneDepthSRV = nullptr; // t41
static bool g_depthIsMSAA = false;

static FogParams g_params{};

struct CB_Fog
{
    DirectX::XMFLOAT4X4 invProjT;
    DirectX::XMFLOAT3   camPosW;
    float               time;

    DirectX::XMFLOAT3   fogColor;
    float               fogDensityNear;

    float               fogDensityFar;
    float               densityRampPow;
    float               fogStart;
    float               fogEnd;

    float               anisotropyG;
    float               heightFalloff;
    DirectX::XMFLOAT2   padFog;

    DirectX::XMFLOAT3   lightDirW;
    float               pad0;
    DirectX::XMFLOAT3   lightColor;
    float               lightIntensity;
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

namespace ShaderFog
{
    bool Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    {
        if (!dev || !ctx) return false;
        g_dev = dev;
        g_ctx = ctx;

        // VS
        std::vector<unsigned char> vsbin;
        if (!LoadCSO("shader_vertex_volumetric_fog.cso", vsbin)) return false;
        if (FAILED(g_dev->CreateVertexShader(vsbin.data(), vsbin.size(), nullptr, &g_vs))) return false;

        // PS (non-MSAA)
        std::vector<unsigned char> psbin;
        if (!LoadCSO("shader_pixel_volumetric_fog.cso", psbin)) return false;
        if (FAILED(g_dev->CreatePixelShader(psbin.data(), psbin.size(), nullptr, &g_ps))) return false;

        // PS (MSAA)
        std::vector<unsigned char> psmsaa;
        if (!LoadCSO("shader_pixel_volumetric_fog_msaa.cso", psmsaa)) return false;
        if (FAILED(g_dev->CreatePixelShader(psmsaa.data(), psmsaa.size(), nullptr, &g_psMSAA))) return false;

        // CB b6
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.ByteWidth = (UINT)((sizeof(CB_Fog) + 15) & ~15);
        if (FAILED(g_dev->CreateBuffer(&bd, nullptr, &g_cbFog))) return false;

        // Sampler s0 clamp linear
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(g_dev->CreateSamplerState(&sd, &g_sClamp))) return false;

        return true;
    }

    void Finalize()
    {
        SAFE_RELEASE(g_sClamp);
        SAFE_RELEASE(g_cbFog);
        SAFE_RELEASE(g_psMSAA);
        SAFE_RELEASE(g_ps);
        SAFE_RELEASE(g_vs);
        g_dev = nullptr;
        g_ctx = nullptr;
    }

    void SetSceneSRV(ID3D11ShaderResourceView* sceneColorCopySRV,
        ID3D11ShaderResourceView* sceneDepthSRV,
        bool depthIsMSAA)
    {
        g_sceneColorSRV = sceneColorCopySRV;
        g_sceneDepthSRV = sceneDepthSRV;
        g_depthIsMSAA = depthIsMSAA;
    }

    void SetParams(const FogParams& p)
    {
        g_params = p;
    }

    void Draw(const XMMATRIX& proj)
    {
        if (!g_ctx || !g_vs || !g_ps || !g_cbFog) return;
        if (!g_sceneColorSRV || !g_sceneDepthSRV) return;

        // ---- 绑定 RT：用只读 DSV（和你水一样，避免 depth SRV/DSV hazard）----
        ID3D11RenderTargetView* rtv = Direct3D_GetMainRTV();
        ID3D11DepthStencilView* dsvRO = Direct3D_GetMainDSV_ReadOnly();
        g_ctx->OMSetRenderTargets(1, &rtv, dsvRO);

        // ---- Shader ----
        g_ctx->VSSetShader(g_vs, nullptr, 0);
        g_ctx->PSSetShader(g_depthIsMSAA ? g_psMSAA : g_ps, nullptr, 0);

        // 全屏三角：不需要 IA layout / VB
        g_ctx->IASetInputLayout(nullptr);
        g_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ---- 常量 b6：invProjT ----
        CB_Fog cb{};
        XMMATRIX invP = XMMatrixInverse(nullptr, proj);
        XMStoreFloat4x4(&cb.invProjT, XMMatrixTranspose(invP));

        cb.camPosW = g_params.camPosW;
        cb.time = g_params.time;

        cb.fogColor = g_params.fogColor;
        cb.fogDensityNear = g_params.fogDensityNear;

        cb.fogDensityFar = g_params.fogDensityFar;
        cb.densityRampPow = g_params.densityRampPow;
        cb.fogStart = g_params.fogStart;
        cb.fogEnd = g_params.fogEnd;

        cb.anisotropyG = g_params.anisotropyG;
        cb.heightFalloff = g_params.heightFalloff;
        cb.padFog = { 0,0 };

        cb.lightDirW = g_params.lightDirW;
        cb.pad0 = 0.0f;
        cb.lightColor = g_params.lightColor;
        cb.lightIntensity = g_params.lightIntensity;

        g_ctx->UpdateSubresource(g_cbFog, 0, nullptr, &cb, 0, 0);
        g_ctx->PSSetConstantBuffers(6, 1, &g_cbFog);

        // ---- 资源 t40/t41 + sampler s0 ----
        g_ctx->PSSetSamplers(0, 1, &g_sClamp);
        g_ctx->PSSetShaderResources(40, 1, &g_sceneColorSRV);
        g_ctx->PSSetShaderResources(41, 1, &g_sceneDepthSRV);

        // fog 是“合成输出”，不需要透明混合
        g_ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

        // Draw(3,0)
        g_ctx->Draw(3, 0);

        // 清掉 SRV（避免后面有人把同资源当 RT/DSV 绑定时报 hazard）
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_ctx->PSSetShaderResources(40, 1, &nullSRV);
        g_ctx->PSSetShaderResources(41, 1, &nullSRV);

        // 恢复主 DSV（可选；你 water 里后面会再设一次）
        ID3D11DepthStencilView* dsv = Direct3D_GetMainDSV();
        g_ctx->OMSetRenderTargets(1, &rtv, dsv);
    }
} // namespace ShaderFog
