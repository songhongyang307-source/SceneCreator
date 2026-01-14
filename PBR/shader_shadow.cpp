/*==============================================================================
   shadow shader[shader_shadow.cpp]
==============================================================================*/

#include "shader_shadow.h"
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

static ID3D11VertexShader* g_pVS = nullptr;
static ID3D11InputLayout* g_pIL = nullptr;

static ID3D11Buffer* g_pVS_CB0_WVP = nullptr;      // VS b0

static ID3D11DepthStencilState* g_pDSShadowWrite = nullptr; // 写深 ON
static ID3D11RasterizerState* g_pRSShadowBias = nullptr; // DepthBias ON

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static bool ReadFileBinary(const char* path, std::vector<unsigned char>& out)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    ifs.seekg(0, std::ios::end);
    size_t size = (size_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    out.resize(size);
    ifs.read((char*)out.data(), size);
    return true;
}

bool ShadowShader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    if (!pDevice || !pContext) return false;
    g_pDevice = pDevice;
    g_pContext = pContext;

    HRESULT hr;

    // ===== VS =====
    std::vector<unsigned char> vsbin;
    if (!ReadFileBinary("Shader_shadow_vertex_3d.cso", vsbin))
        return false;

    hr = g_pDevice->CreateVertexShader(vsbin.data(), vsbin.size(), nullptr, &g_pVS);
    if (FAILED(hr)) return false;

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
        vsbin.data(), vsbin.size(), &g_pIL);
    if (FAILED(hr)) return false;

    // ===== VS CB b0 (WVP) =====
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = sizeof(XMFLOAT4X4);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        hr = g_pDevice->CreateBuffer(&bd, nullptr, &g_pVS_CB0_WVP);
        if (FAILED(hr)) return false;
    }

    // ===== DepthState（ShadowMap：写深 ON）=====
    {
        D3D11_DEPTH_STENCIL_DESC dd{};
        dd.DepthEnable = TRUE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;    // ✅ 必须写深
        dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        hr = g_pDevice->CreateDepthStencilState(&dd, &g_pDSShadowWrite);
        if (FAILED(hr)) return false;
    }

    // ===== Rasterizer（ShadowMap：CullBack + DepthBias）=====
    {
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_BACK;                     // ✅ 常用（更干净）
        rd.DepthBias = 100;                                // ✅ 起步值（后面可调）
        rd.SlopeScaledDepthBias = 2.0f;                    // ✅ 起步值（后面可调）
        rd.DepthBiasClamp = 0.0f;
        hr = g_pDevice->CreateRasterizerState(&rd, &g_pRSShadowBias);
        if (FAILED(hr)) return false;
    }

    return true;
}

void ShadowShader_Finalize()
{
    if (g_pRSShadowBias) { g_pRSShadowBias->Release();   g_pRSShadowBias = nullptr; }
    if (g_pDSShadowWrite) { g_pDSShadowWrite->Release();  g_pDSShadowWrite = nullptr; }

    if (g_pVS_CB0_WVP) { g_pVS_CB0_WVP->Release(); g_pVS_CB0_WVP = nullptr; }

    if (g_pIL) { g_pIL->Release(); g_pIL = nullptr; }
    if (g_pVS) { g_pVS->Release(); g_pVS = nullptr; }

    g_pDevice = nullptr;
    g_pContext = nullptr;
}

void ShadowShader_Begin()
{
    g_pContext->VSSetShader(g_pVS, nullptr, 0);
    g_pContext->PSSetShader(nullptr, nullptr, 0); // ✅ 深度pass不需要PS

    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pContext->IASetInputLayout(g_pIL);

    g_pContext->VSSetConstantBuffers(0, 1, &g_pVS_CB0_WVP);

    g_pContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF); // ✅ 关闭混合
    g_pContext->OMSetDepthStencilState(g_pDSShadowWrite, 0);   // ✅ 写深 ON
    g_pContext->RSSetState(g_pRSShadowBias);                   // ✅ Bias ON
}

void ShadowShader_End()
{
    g_pContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    g_pContext->OMSetDepthStencilState(nullptr, 0);
    g_pContext->RSSetState(nullptr);
}

void ShadowShader_SetWVPMatrix(const XMMATRIX& wvp)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(wvp));
    g_pContext->UpdateSubresource(g_pVS_CB0_WVP, 0, nullptr, &t, 0, 0);
}

