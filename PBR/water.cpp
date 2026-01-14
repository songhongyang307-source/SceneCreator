#include "water.h"
#include "shader_water.h"
#include "direct3d.h"
#include "texture.h"
#include "model.h"
#include "shader3d .h"

static float g_time = 0.0f;
static XMMATRIX g_world = XMMatrixIdentity();
static int g_n0 = -1, g_n1 = -1;
static ID3D11RasterizerState* g_RS_CullNone = nullptr;
static ID3D11DepthStencilState* g_DSS_DepthAlways = nullptr;

static WaterTransform g_WaterTf;
extern DirectX::XMMATRIX g_world; // 你 water.cpp 里原本就有的 world（如果是 static，就改成提供 setter）

static ID3D11DepthStencilState* g_DSS_DepthTestNoWrite = nullptr;

WaterTransform* Water_GetTransformPtr()
{
    return &g_WaterTf;
}
static XMMATRIX BuildWorldFromTf()
{
    XMMATRIX S = XMMatrixScaling(g_WaterTf.scale.x, g_WaterTf.scale.y, g_WaterTf.scale.z);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(g_WaterTf.rotation.x, g_WaterTf.rotation.y, g_WaterTf.rotation.z);
    XMMATRIX T = XMMatrixTranslation(g_WaterTf.position.x, g_WaterTf.position.y, g_WaterTf.position.z);
    return S * R * T;
}
struct WaterVS_CB
{
    DirectX::XMMATRIX mtxWorld;
    DirectX::XMMATRIX mtxView;
    DirectX::XMMATRIX mtxProj;
};
static ID3D11Buffer* g_cbWaterVS = nullptr;

static void DBG_DepthMatch(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* depthSRV)
{
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);

    ID3D11Resource* srvRes = nullptr;
    ID3D11Resource* dsvRes = nullptr;
    if (depthSRV) depthSRV->GetResource(&srvRes);
    if (dsv) dsv->GetResource(&dsvRes);

    DBG("[Water] OM DSV=%p  SRVRes=%p  DSVRes=%p  match=%d",
        dsv, srvRes, dsvRes, (srvRes == dsvRes) ? 1 : 0);

    SAFE_RELEASE(rtv);
    SAFE_RELEASE(dsv);
    SAFE_RELEASE(srvRes);
    SAFE_RELEASE(dsvRes);
}
static void ExtractNearFarLH(const XMMATRIX& proj, float& outNearZ, float& outFarZ)
{
    XMFLOAT4X4 P{};
    XMStoreFloat4x4(&P, proj);

    // LH perspective:
    // P._33 = f/(f-n)
    // P._43 = -n*f/(f-n)
    const float a = P._33;
    const float b = P._43;

    outNearZ = -b / a;
    outFarZ = b / (1.0f - a);
}
// 这里你可以：1) 自己做一张平面VB/IB；2) 用你现有的 MODEL 但自己 draw VB/IB
static MODEL* g_waterPlane = nullptr;



DirectX::XMMATRIX Water_GetWorldMatrix()
{
    return BuildWorldFromTf();
}

void Water_ApplyTransformNow()
{
    g_world = BuildWorldFromTf();
}

bool Water_Pick(const Ray& ray, DirectX::XMFLOAT3* outHitW)
{
    const XMMATRIX W = BuildWorldFromTf();
    const XMMATRIX invW = XMMatrixInverse(nullptr, W);

    XMVECTOR roL = XMVector3TransformCoord(XMLoadFloat3(&ray.origin), invW);
    XMVECTOR rdL = XMVector3TransformNormal(XMLoadFloat3(&ray.dir), invW);
    rdL = XMVector3Normalize(rdL);

    float roY = XMVectorGetY(roL);
    float rdY = XMVectorGetY(rdL);
    if (fabsf(rdY) < 1e-6f) return false;

    float t = -roY / rdY;
    if (t <= 0.0f) return false;

    XMVECTOR hitL = roL + rdL * t;

    // ✅ 限制拾取范围：水平面局部尺寸（按你的水网格大小调）
    // 如果点不到水，就把 half 从 0.5f 调大到 5 / 50 试
    const float half = 50.0f;
    float hx = XMVectorGetX(hitL);
    float hz = XMVectorGetZ(hitL);
    if (fabsf(hx) > half || fabsf(hz) > half) return false;

    if (outHitW)
    {
        XMVECTOR hitW = XMVector3TransformCoord(hitL, W);
        XMStoreFloat3(outHitW, hitW);
    }
    return true;
}
static bool CreateWaterDepthState(ID3D11Device* dev)
{
    D3D11_DEPTH_STENCIL_DESC ds{};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // 不写深度
    ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;       // 关键：按深度挡住
    ds.StencilEnable = FALSE;

    return SUCCEEDED(dev->CreateDepthStencilState(&ds, &g_DSS_DepthTestNoWrite));
}

bool Water_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    if (!ShaderWater_Initialize(dev, ctx)) return false;
    if (!CreateWaterDepthState(dev)) return false;
    // ---- Rasterizer: Cull None ----
    {
        D3D11_RASTERIZER_DESC rs{};
        rs.FillMode = D3D11_FILL_SOLID;
        rs.CullMode = D3D11_CULL_NONE;      // 关键：不剔除
        rs.FrontCounterClockwise = FALSE;
        rs.DepthClipEnable = TRUE;

        HRESULT hr = dev->CreateRasterizerState(&rs, &g_RS_CullNone);
        if (FAILED(hr)) return false;
    }

    // ---- DepthStencil: Depth Always ----
    {
        D3D11_DEPTH_STENCIL_DESC ds{};
        ds.DepthEnable = TRUE;
        ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 不写深度，避免影响后续
        ds.DepthFunc = D3D11_COMPARISON_ALWAYS;          // 关键：永远通过
        ds.StencilEnable = FALSE;

        HRESULT hr = dev->CreateDepthStencilState(&ds, &g_DSS_DepthAlways);
        if (FAILED(hr)) return false;
    }
    g_n0 = Texture_Load(L"resource/texture/Water_nrm01.jpg", false);
    g_n1 = Texture_Load(L"resource/texture/Water_nrm02.jpg", false);
    ShaderWater_SetNormalTextures(g_n0, g_n1);

    // 你可以加载一个 plane 模型
    g_waterPlane = ModelLoad("resource/mesh/Plane.fbx", false, 1.0f, true);

    g_world = XMMatrixTranslation(0.0f, 0.1f, 3.0f);
    return true;
}

void Water_Finalize()
{
    if (g_waterPlane) { ModelRelease(g_waterPlane); g_waterPlane = nullptr; }
    ShaderWater_Finalize();
    if (g_RS_CullNone) { g_RS_CullNone->Release(); g_RS_CullNone = nullptr; }
    if (g_DSS_DepthAlways) { g_DSS_DepthAlways->Release(); g_DSS_DepthAlways = nullptr; }
    SAFE_RELEASE(g_DSS_DepthTestNoWrite);
}

void Water_Update(float dt) { g_time += dt; }

void Water_Draw(const XMMATRIX& view, const XMMATRIX& proj, const XMFLOAT3& eyePosW)
{
    auto* ctx = Direct3D_GetContext();

    Direct3D_PrepareWaterInputs();

   // 1) 传 scene SRV（t40/t41）
    const bool msaa = Direct3D_UseMSAA();
    ID3D11ShaderResourceView* depthSRV = Direct3D_GetDepthSRV();

    ShaderWater_SetSceneSRV(
        Direct3D_GetSceneColorCopySRV(),
        depthSRV,
        msaa
    );

    ShaderWater_SetIBL(
        Shader3d_GetPrefilterSRV(),
        Shader3d_GetBRDFLUTSRV(),
        Shader3d_GetPrefilterMaxMip()
    );

    // 2) 水参数
    WaterParams p{};
    p.eyePosW = eyePosW;
    p.time = g_time;

    p.refractStrength = 0.006f;
    p.absorptionRGB = { 0.12f, 0.05f, 0.02f };
    p.roughness = 0.05f;
    p.foamFadeDist = 0.7f;
    p.envIntensityWater = 0.5f;
    p.foamStrength = 0.15f;

    p.n0Tiling = { 4.0f, 4.0f };
    p.n1Tiling = { 8.0f, 8.0f };
    p.n0Speed = { 0.03f, 0.02f };
    p.n1Speed = { -0.02f, 0.04f };

    // ★3) near/far 只算一次：把你原来的 “0.5/100 + ExtractNearFarLH + 又手算一次” 全删掉
    XMFLOAT4X4 pf{};
    XMStoreFloat4x4(&pf, proj);
    float A = pf._33;
    float B = pf._43;
    p.nearZ = -B / A;
    p.farZ = (A * p.nearZ) / (A - 1.0f);
    // DBG("[Water] nearZ=%.3f farZ=%.3f", p.nearZ, p.farZ);

    ShaderWater_SetParams(p);

    // 保存旧状态：一定要在 ShaderWater_Begin 之前保存（你原来是 Begin 后才保存，等于白保存）
    ID3D11RasterizerState* oldRS = nullptr;
    ID3D11BlendState* oldBS = nullptr;
    ID3D11DepthStencilState* oldDSS = nullptr;
    float oldBF[4] = { 0,0,0,0 };
    UINT  oldMask = 0;
    UINT  oldRef = 0;

    ctx->RSGetState(&oldRS);
    ctx->OMGetBlendState(&oldBS, oldBF, &oldMask);
    ctx->OMGetDepthStencilState(&oldDSS, &oldRef);

    // 5) 水 pass：只绑 RTV，不绑 DSV（避免 SRV/DSV hazard）
    ID3D11RenderTargetView* rtv = Direct3D_GetMainRTV();
    ID3D11DepthStencilView* dsvRO = Direct3D_GetMainDSV_ReadOnly();
    ctx->OMSetRenderTargets(1, &rtv, dsvRO);

    ctx->OMSetDepthStencilState(g_DSS_DepthTestNoWrite, 0);
    // ★6) 如果你水面是双面都要看到，才设置 CullNone（可以留）
    //ctx->RSSetState(g_RS_CullNone);

    // ★7) Begin 里会设置：水用的 VS/PS、CB、贴图、sampler、以及 alpha blend / depth disable
    //    注意：waterWorld 要和你想摆的位置一致（别一会儿用 g_world 一会儿 translation(0,0,10)）
    XMMATRIX waterWorld = g_world; // 推荐统一用 g_world
    ShaderWater_Begin(waterWorld, view, proj);

    if (!g_waterPlane) { DBG("waterPlane is null"); goto RESTORE; }
    ModelDrawShadowRaw(g_waterPlane);

    ShaderWater_End();

RESTORE:
    // ★8) 恢复旧状态（避免影响后面画 sprite / bullet / effect）
    ctx->RSSetState(oldRS);
    ctx->OMSetBlendState(oldBS, oldBF, oldMask);
    ctx->OMSetDepthStencilState(oldDSS, oldRef);

    SAFE_RELEASE(oldRS);
    SAFE_RELEASE(oldBS);
    SAFE_RELEASE(oldDSS);

    // 9) 恢复主渲染（重新绑回 DSV）
    ID3D11DepthStencilView* dsv = Direct3D_GetMainDSV();
    ctx->OMSetRenderTargets(1, &rtv, dsv);
    Direct3D_SetDepthEnable(true);
}
