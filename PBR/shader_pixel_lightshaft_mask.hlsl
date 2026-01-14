#ifndef SHAFT_MSAA
#define SHAFT_MSAA 0
#endif

#ifndef MSAA_SAMPLES
#define MSAA_SAMPLES 4
#endif

// depth t41
#if SHAFT_MSAA
Texture2DMS<float> g_SceneDepth : register(t41);

float LoadDepthMin(int2 pix)
{
    float d = 1.0f;
    [unroll] for (int i = 0; i < MSAA_SAMPLES; ++i)
        d = min(d, g_SceneDepth.Load(pix, i));
    return d;
}
#else
Texture2D<float> g_SceneDepth : register(t41);

float LoadDepthMin(int2 pix)
{
    return g_SceneDepth.Load(int3(pix, 0));
}
#endif

SamplerState sampClamp : register(s0);

// b7：必须和 cpp 的 CB_Shaft 完全一致
cbuffer CB_Shaft : register(b7)
{
    float4x4 g_InvProj; // transpose(invProj) 已在 C++ 里做过
    float2 g_SunUV;
    float g_Intensity;
    float g_SkyDepthEps;

    float3 g_SunDirV; // view space sun dir (normalized)
    float g_SunThreshold;

    float g_SunPower;
    float g_Density;
    float g_Decay;
    float g_Weight;

    float g_Exposure;
    float2 g_InvTargetSize;
    float g_Pad0;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float3 ReconstructViewDir(float2 uv)
{
    // 用 far(=1) 重建“视线方向”，不受场景深度影响
    float4 ndc = float4(uv * 2.0f - 1.0f, 1.0f, 1.0f);
    ndc.y = -ndc.y;

    float4 v = mul(ndc, g_InvProj); // invProjT
    float3 p = v.xyz / max(v.w, 1e-6);
    return normalize(p);
}

float4 PS_LightShaftMask(VSOut pin) : SV_Target
{
    int2 pix = int2(pin.pos.xy);
    float depth01 = LoadDepthMin(pix);

    // 只有天空才“发光”，几何体为 0（遮挡）
    bool isSky = (depth01 >= g_SkyDepthEps);
    if (!isSky)
        return float4(0, 0, 0, 1);

    float3 V = ReconstructViewDir(pin.uv);
    float3 S = normalize(g_SunDirV);

    // 太阳在背后时直接关掉（可选，但很稳）
    if (S.z <= 0.0f)
        return float4(0, 0, 0, 1);

    float cosTheta = dot(V, S);

    // threshold 越接近 1，太阳盘越小
    float x = saturate((cosTheta - g_SunThreshold) / max(1.0f - g_SunThreshold, 1e-3));
    float sunDisk = pow(x, max(g_SunPower, 1e-3));

    // Mask 里先不乘 intensity，后面 blur/comp 再乘（更好调）
    return float4(sunDisk, 0, 0, 1);
}
