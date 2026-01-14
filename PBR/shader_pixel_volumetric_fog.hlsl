
/*==============================================================================

  shader_pixel_volumetric_fog[shader_pixel_volumetric_fog.hlsl]
                                                     Author : The shy
                                                     Date   : 2026/01/09
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef FOG_MSAA
#define FOG_MSAA 0
#endif

#ifndef MSAA_SAMPLES
#define MSAA_SAMPLES 4
#endif

// ===== Textures =====
Texture2D g_SceneColor : register(t40);

#if FOG_MSAA
Texture2DMS<float> g_SceneDepth : register(t41);

float LoadSceneDepth_Min(int2 pix)
{
    float d = 1.0f;
    [unroll] for (int i = 0; i < MSAA_SAMPLES; ++i)
        d = min(d, g_SceneDepth.Load(pix, i));
    return d;
}
#else
Texture2D<float> g_SceneDepth : register(t41);

float LoadSceneDepth_Min(int2 pix)
{
    return g_SceneDepth.Load(int3(pix, 0));
}
#endif

// ===== Samplers =====
SamplerState sampClamp : register(s0);

// ===== Constant Buffers =====
cbuffer CB_Fog : register(b6)
{
    float4x4 g_InvProj; // 注意：C++ 上传 transpose(invProj)，所以 HLSL 用 mul(v, M)
    float3 g_CamPosW;
    float g_Time;

    float3 g_FogColor;
    float g_FogDensityNear; // 新：近处密度

    float g_FogDensityFar; // 新：远处密度
    float g_DensityRampPow; // 新：曲线指数（1~4 常用）
    float g_FogStart;
    float g_FogEnd;

    float g_AnisotropyG;
    float g_HeightFalloff;
    float2 g_PadFog;

    float3 g_LightDirW;
    float g_Pad0;
    float3 g_LightColor;
    float g_LightIntensity;
};

// ===== Helpers =====
float3 ReconstructViewPos(float2 uv, float depth01)
{
    // ★完全照你 water 的写法
    float4 ndc = float4(uv * 2.0f - 1.0f, depth01, 1.0f);
    ndc.y = -ndc.y;

    float4 v = mul(ndc, g_InvProj); // ★因为 g_InvProj 实际是 invProjT
    return v.xyz / max(v.w, 1e-6);
}

float PhaseHG(float cosTheta, float g)
{
    float gg = g * g;
    float denom = pow(1.0 + gg - 2.0 * g * cosTheta, 1.5);
    return (1.0 - gg) / max(4.0 * 3.14159265 * denom, 1e-6);
}

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// ===== Fullscreen VS =====
struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut VS_Fullscreen(uint id : SV_VertexID)
{
    VSOut o;
    float2 p;
    if (id == 0)
        p = float2(-1.0, -1.0);
    else if (id == 1)
        p = float2(-1.0, 3.0);
    else
        p = float2(3.0, -1.0);

    o.pos = float4(p, 0.0, 1.0);

    // 这套 uv 写法一般 OK；如果你发现上下颠倒，就把 y 改成 (p.y+1)*0.5
    o.uv = float2((p.x + 1.0) * 0.5, (1.0 - p.y) * 0.5);
    return o;
}

// ===== Main PS =====
float4 PS_VolumetricFog(VSOut pin) : SV_TARGET
{
    float2 uv = pin.uv;
    float3 scene = g_SceneColor.Sample(sampClamp, uv).rgb;

    // 读最前面的深度
    int2 pix = int2(pin.pos.xy);
    float depth01 = LoadSceneDepth_Min(pix);

    // 用 depth01 重建 viewPos（天空/无几何一般 depth01=1）
    float3 viewPos = ReconstructViewPos(uv, depth01);
    float3 viewDir = normalize(viewPos);

    // 沿视线距离（viewPos 与 viewDir 同向时，length 就是 ray distance）
    float viewDist = length(viewPos);

    // depth=1 的天空：用 g_FogEnd 当作最大积分距离
    bool isSky = (depth01 >= 0.9999f);

    
    float D = isSky ? min(g_FogEnd, 20.0f) : min(viewDist, g_FogEnd);
    if (D <= g_FogStart)
        return float4(scene, 1);

    const int STEPS = 48;
    float t0 = g_FogStart;
    float t1 = D;

    // 每像素抖动减少条纹
    float jitter = Hash12(pin.pos.xy + g_Time); // [0,1)
    float dt = (t1 - t0) / STEPS;
    float t = t0 + dt * jitter;

    float3 fogAccum = 0.0;
    float trans = 1.0;

    float3 lightDir = normalize(-g_LightDirW);
    float phase = PhaseHG(dot(viewDir, lightDir), g_AnisotropyG);
    float skyScale = isSky ? 0.25f : 1.0f; // 0.1~0.4 
    [loop]
    for (int i = 0; i < STEPS; ++i)
    {
        
        // ---- 距离归一化：控制“从近到远变浓” ----
        float uAbs = saturate((t - g_FogStart) / max(g_FogEnd - g_FogStart, 1e-3));
        uAbs = pow(uAbs, max(g_DensityRampPow, 1e-3));

        float density = lerp(g_FogDensityNear, g_FogDensityFar, uAbs);

// ✅ 关键：近处清晰区（用距离把密度淡入）
// 先写死，稳定后再做成参数
        const float clearStart = 0.0f;
        const float clearEnd = 12.0f; // 8~20 之间调
        float nearFade = smoothstep(clearStart, clearEnd, t);
        density *= nearFade;

// ✅ 天空单独压（避免天空被雾染脏）
        float skyScale = isSky ? 0.08f : 1.0f; // 0.05~0.15 调
        density *= skyScale;

// 散射/消光
        float sigmaT = density;
        float sigmaS = density * 0.8f; // 0.6~0.95 可调

        float3 Lin = g_FogColor + g_LightColor * (g_LightIntensity * phase);
        fogAccum += trans * (sigmaS * Lin) * dt;
        trans *= exp(-sigmaT * dt);

        if (trans < 0.01)
            break;
        t += dt;
    }

    float3 outCol = scene * trans + fogAccum;
    return float4(outCol, 1);
}
