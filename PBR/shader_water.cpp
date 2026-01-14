/*==============================================================================

   water shader [shader_water.cpp]
                                                         Author : the shy
                                                         Date   : 2025/12/28
--------------------------------------------------------------------------------

==============================================================================*/




#include "shader_water.h"
#include "direct3d.h"
#include "texture.h"
#include <fstream>
#include <vector>
#include "shader3d .h"



// 你工程里一般有 SAFE_RELEASE；没有的话加一个
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) do{ if(x){ (x)->Release(); (x)=nullptr; } }while(0)
#endif

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;

static ID3D11VertexShader* g_vs = nullptr;
static ID3D11PixelShader* g_ps = nullptr;
static ID3D11PixelShader* g_psMSAA = nullptr;
static ID3D11InputLayout* g_layout = nullptr;

static ID3D11Buffer* g_cbVSWorld = nullptr; // b0
static ID3D11Buffer* g_cbVSView = nullptr; // b1
static ID3D11Buffer* g_cbVSProj = nullptr; // b2
static ID3D11Buffer* g_cbPSWater = nullptr; // b6
static ID3D11Buffer* g_cbIBL = nullptr; // b8

static ID3D11SamplerState* g_sClamp = nullptr; // s0
static ID3D11SamplerState* g_sWrap = nullptr; // s1
static ID3D11SamplerState* g_sIBL = nullptr; // s2

static int g_texN0 = -1; // t10
static int g_texN1 = -1; // t11

static ID3D11ShaderResourceView* g_sceneColorSRV = nullptr; // t40 (外部持有)
static ID3D11ShaderResourceView* g_sceneDepthSRV = nullptr; // t41 (外部持有)
static bool g_depthIsMSAA = true;

static ID3D11ShaderResourceView* g_prefilterSRV = nullptr;  // t31 (外部持有)
static ID3D11ShaderResourceView* g_brdfLutSRV = nullptr;  // t32 (外部持有)
static float g_prefilterMaxMip = 0.0f;

static ID3D11DepthStencilState* g_dssDepthTestNoWrite = nullptr;
static WaterParams g_params{};




struct CB_WaterPS
{
    XMFLOAT4X4 invProjT;     // transpose(invProj)
    XMFLOAT3   eyePosW;
    float      time;

    XMFLOAT3   absorptionRGB;
    float      refractStrength;

    XMFLOAT2   texelSize;
    XMFLOAT2   pad0;

    float roughness;
    float foamFadeDist;
    float envIntensityWater;
    float foamStrength;

    XMFLOAT2 n0Tiling;
    XMFLOAT2 n0Speed;
    XMFLOAT2 n1Tiling;
    XMFLOAT2 n1Speed;

    float nearZ;
    float farZ;
    XMFLOAT2 padNF;
};

struct CB_IBL
{
    float PrefilterMaxMip;
    float EnvIntensity; // 这里水PS不一定用，但留着对齐
    float pad[2];
};

static bool CreateSamplers()
{
    // s0 clamp linear
    {
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(g_dev->CreateSamplerState(&sd, &g_sClamp))) return false;
    }
    // s1 wrap anisotropic
    {
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_ANISOTROPIC;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.MaxAnisotropy = 8;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(g_dev->CreateSamplerState(&sd, &g_sWrap))) return false;
    }
    // s2 IBL clamp (linear)
    {
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(g_dev->CreateSamplerState(&sd, &g_sIBL))) return false;
    }
    return true;
}

static bool LoadCSO(const char* path, std::vector<unsigned char>& out)
{
    char cwd[MAX_PATH]{};
    GetCurrentDirectoryA(MAX_PATH, cwd);
    DBG("[WaterInit] CWD = %s", cwd);

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        DBG("[WaterInit] LoadCSO failed: %s", path);
        return false;
    }
    ifs.seekg(0, std::ios::end);
    size_t sz = (size_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    out.resize(sz);
    ifs.read((char*)out.data(), sz);
    DBG("[WaterInit] LoadCSO ok: %s (%zu bytes)", path, sz);
    return true;
}

#define HR_CHECK(hr, msg) do{ \
    if(FAILED(hr)){ DBG("[WaterInit] %s failed hr=0x%08X", msg, (unsigned)(hr)); return false; } \
}while(0)


bool ShaderWater_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    DBG("[WaterInit] enter dev=%p ctx=%p", dev, ctx);
    if (!dev || !ctx) return false;
    g_dev = dev;
    g_ctx = ctx;

    // --- VS ---
    std::vector<unsigned char> vsbin;
    if (!LoadCSO("shader_vertex_water.cso", vsbin)) return false;

    HRESULT hr = g_dev->CreateVertexShader(vsbin.data(), vsbin.size(), nullptr, &g_vs);
    HR_CHECK(hr, "CreateVertexShader");

    // input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        // ★这里很容易错：如果你的 VS 里 TANGENT 是 float4，必须改成 R32G32B32A32
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = g_dev->CreateInputLayout(layout, ARRAYSIZE(layout), vsbin.data(), vsbin.size(), &g_layout);
    HR_CHECK(hr, "CreateInputLayout");

    // --- PS ---
    std::vector<unsigned char> psbin;
    if (!LoadCSO("shader_pixel_water.cso", psbin)) return false;
    hr = g_dev->CreatePixelShader(psbin.data(), psbin.size(), nullptr, &g_ps);
    HR_CHECK(hr, "CreatePixelShader water");

    std::vector<unsigned char> psmsaa;
    if (!LoadCSO("shader_pixel_water_msaa.cso", psmsaa)) return false;
    hr = g_dev->CreatePixelShader(psmsaa.data(), psmsaa.size(), nullptr, &g_psMSAA);
    HR_CHECK(hr, "CreatePixelShader water msaa");

    // --- Constant Buffers ---
    DBG("[WaterInit] sizeof(CB_WaterPS)=%u", (unsigned)sizeof(CB_WaterPS));

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    bd.ByteWidth = sizeof(XMFLOAT4X4);
    hr = g_dev->CreateBuffer(&bd, nullptr, &g_cbVSWorld); HR_CHECK(hr, "CreateBuffer VSWorld");
    hr = g_dev->CreateBuffer(&bd, nullptr, &g_cbVSView);  HR_CHECK(hr, "CreateBuffer VSView");
    hr = g_dev->CreateBuffer(&bd, nullptr, &g_cbVSProj);  HR_CHECK(hr, "CreateBuffer VSProj");

    bd.ByteWidth = (UINT)((sizeof(CB_WaterPS) + 15) & ~15); // ★保险起见 16 对齐
    hr = g_dev->CreateBuffer(&bd, nullptr, &g_cbPSWater); HR_CHECK(hr, "CreateBuffer PSWater");

    bd.ByteWidth = sizeof(CB_IBL);
    hr = g_dev->CreateBuffer(&bd, nullptr, &g_cbIBL); HR_CHECK(hr, "CreateBuffer IBL");

    if (!CreateSamplers()) { DBG("[WaterInit] CreateSamplers failed"); return false; }

    // Depth test ON, depth write OFF
    {
        D3D11_DEPTH_STENCIL_DESC ds{};
        ds.DepthEnable = TRUE;
        ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        ds.StencilEnable = FALSE;

        HRESULT hr = g_dev->CreateDepthStencilState(&ds, &g_dssDepthTestNoWrite);
        HR_CHECK(hr, "CreateDepthStencilState water no-write");
    }

    g_params.texelSize = {
        1.0f / (float)Direct3D_GetBackBufferWidth(),
        1.0f / (float)Direct3D_GetBackBufferHeight()
    };

    DBG("[WaterInit] success");


    return true;
}

void ShaderWater_Finalize()
{
    SAFE_RELEASE(g_sClamp);
    SAFE_RELEASE(g_sWrap);
    SAFE_RELEASE(g_sIBL);

    SAFE_RELEASE(g_cbIBL);
    SAFE_RELEASE(g_cbPSWater);

    SAFE_RELEASE(g_cbVSProj);
    SAFE_RELEASE(g_cbVSView);
    SAFE_RELEASE(g_cbVSWorld);

    SAFE_RELEASE(g_psMSAA);
    SAFE_RELEASE(g_ps);
    SAFE_RELEASE(g_layout);
    SAFE_RELEASE(g_vs);
    SAFE_RELEASE(g_dssDepthTestNoWrite);


    g_dev = nullptr;
    g_ctx = nullptr;
}

void ShaderWater_SetNormalTextures(int texN0, int texN1)
{
    g_texN0 = texN0;
    g_texN1 = texN1;
}

void ShaderWater_SetSceneSRV(ID3D11ShaderResourceView* sceneColorCopy, ID3D11ShaderResourceView* sceneDepth, bool depthIsMSAA)
{
    g_sceneColorSRV = sceneColorCopy;
    g_sceneDepthSRV = sceneDepth;
    g_depthIsMSAA = depthIsMSAA;
}

void ShaderWater_SetIBL(ID3D11ShaderResourceView* prefilter, ID3D11ShaderResourceView* brdfLut, float prefilterMaxMip)
{
    g_prefilterSRV = prefilter;
    g_brdfLutSRV = brdfLut;
    g_prefilterMaxMip = prefilterMaxMip;
}

void ShaderWater_SetParams(const WaterParams& p)
{
    g_params = p;
    g_params.texelSize = {
        1.0f / (float)Direct3D_GetBackBufferWidth(),
        1.0f / (float)Direct3D_GetBackBufferHeight()
    };
}

void ShaderWater_Begin(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj)
{
    ID3D11ShaderResourceView* nullSRV = nullptr;
    g_ctx->PSSetShaderResources(41, 1, &nullSRV);

    // --- shaders ---
    g_ctx->VSSetShader(g_vs, nullptr, 0);
    g_ctx->PSSetShader(g_depthIsMSAA ? g_psMSAA : g_ps, nullptr, 0);
    g_ctx->IASetInputLayout(g_layout);

    DBG("[Water] depthIsMSAA=%d  depthSRV=%p", (int)g_depthIsMSAA, g_sceneDepthSRV);

    if (g_sceneDepthSRV)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC d{};
        g_sceneDepthSRV->GetDesc(&d);
        DBG("[Water] depthSRV ViewDim=%d Format=%d", (int)d.ViewDimension, (int)d.Format);
    }

    // --- VS b0 world (transpose) ---
    

    // ===== 写入 VS 矩阵（必须！）=====
    XMFLOAT4X4 wT, vT, pT;
    XMStoreFloat4x4(&wT, XMMatrixTranspose(world));
    XMStoreFloat4x4(&vT, XMMatrixTranspose(view));
    XMStoreFloat4x4(&pT, XMMatrixTranspose(proj));

    g_ctx->UpdateSubresource(g_cbVSWorld, 0, nullptr, &wT, 0, 0);
    g_ctx->UpdateSubresource(g_cbVSView, 0, nullptr, &vT, 0, 0);
    g_ctx->UpdateSubresource(g_cbVSProj, 0, nullptr, &pT, 0, 0);

    ID3D11Buffer* vsCBs[] = { g_cbVSWorld, g_cbVSView, g_cbVSProj };
    g_ctx->VSSetConstantBuffers(0, 3, vsCBs);

    // --- PS b6 water params ---
    CB_WaterPS cb{};
    XMMATRIX invP = XMMatrixInverse(nullptr, proj);
    XMStoreFloat4x4(&cb.invProjT, XMMatrixTranspose(invP)); // 与你 mul(ndc, invProj) 的习惯保持一致

    cb.eyePosW = g_params.eyePosW;
    cb.time = g_params.time;

    cb.absorptionRGB = g_params.absorptionRGB;
    cb.refractStrength = g_params.refractStrength;

    cb.texelSize = g_params.texelSize;
    cb.pad0 = { 0,0 };

    cb.roughness = g_params.roughness;
    cb.foamFadeDist = g_params.foamFadeDist;
    cb.envIntensityWater = g_params.envIntensityWater;
    cb.foamStrength = g_params.foamStrength;

    cb.n0Tiling = g_params.n0Tiling;
    cb.n0Speed = g_params.n0Speed;
    cb.n1Tiling = g_params.n1Tiling;
    cb.n1Speed = g_params.n1Speed;

    cb.nearZ = g_params.nearZ;
    cb.farZ = g_params.farZ;
    cb.padNF = { 0, 0 };

    if (!g_ctx || !g_cbPSWater) { DBG("Water Begin: NULL ctx/cb"); return; }

    g_ctx->UpdateSubresource(g_cbPSWater, 0, nullptr, &cb, 0, 0);
    g_ctx->PSSetConstantBuffers(6, 1, &g_cbPSWater);

    // --- PS b8 IBL (只给 PrefilterMaxMip) ---
    CB_IBL ibl{};
    ibl.PrefilterMaxMip = g_prefilterMaxMip;
    ibl.EnvIntensity = 1.0f;
    g_ctx->UpdateSubresource(g_cbIBL, 0, nullptr, &ibl, 0, 0);
    g_ctx->PSSetConstantBuffers(8, 1, &g_cbIBL);

    // --- samplers s0/s1/s2 ---
    g_ctx->PSSetSamplers(0, 1, &g_sClamp);
    g_ctx->PSSetSamplers(1, 1, &g_sWrap);
    g_ctx->PSSetSamplers(2, 1, &g_sIBL);

    // --- textures ---
    if (g_texN0 >= 0) Texture_SetTexture(g_texN0, 10);
    if (g_texN1 >= 0) Texture_SetTexture(g_texN1, 11);

    if (g_sceneColorSRV) g_ctx->PSSetShaderResources(40, 1, &g_sceneColorSRV);
    if (g_sceneDepthSRV) g_ctx->PSSetShaderResources(41, 1, &g_sceneDepthSRV);

    if (g_prefilterSRV) g_ctx->PSSetShaderResources(31, 1, &g_prefilterSRV);
    if (g_brdfLutSRV)   g_ctx->PSSetShaderResources(32, 1, &g_brdfLutSRV);

    // 水建议：透明混合 + 关闭深度（深度在 PS 里手动 discard）
    Direct3D_SetAlphaBlendTransparent();
    g_ctx->OMSetDepthStencilState(g_dssDepthTestNoWrite, 0);
}

void ShaderWater_End()
{
    ID3D11ShaderResourceView* nullSRV = nullptr;
    g_ctx->PSSetShaderResources(40, 1, &nullSRV);
    g_ctx->PSSetShaderResources(41, 1, &nullSRV);

 
}
