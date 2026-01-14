
/*==============================================================================

  shader_pixel_water[shader_pixel_water.hlsl]
														 Author : The shy
														 Date   : 2025/12/24
--------------------------------------------------------------------------------

==============================================================================*/






#ifndef WATER_MSAA
#define WATER_MSAA 0
#endif

#ifndef MSAA_SAMPLES
#define MSAA_SAMPLES 4
#endif

// ===== Textures =====
Texture2D g_Normal0 : register(t10);
Texture2D g_Normal1 : register(t11);

// t40：scene color copy（你已 resolve 成非 MSAA 的 Texture2D）
Texture2D g_SceneColor : register(t40);

// t41：scene depth（根据是否 MSAA 切换）
#if WATER_MSAA
Texture2DMS<float> g_SceneDepth : register(t41);

float LoadSceneDepth_Min(int2 pix)
{
    float d = 1.0f;
    [unroll] for (int i = 0; i < MSAA_SAMPLES; ++i)
        d = min(d, g_SceneDepth.Load(pix, i));
    return d;
}

// 返回：水后面的“最近一层”深度（找不到就返回 waterDepth01）
float LoadSceneDepth_BehindWater(int2 pix, float waterDepth01)
{
    float best = 1.0f;
    const float eps = 1e-4f;

    [unroll] for (int i = 0; i < MSAA_SAMPLES; ++i)
    {
        float s = g_SceneDepth.Load(pix, i);

        // ✅ 只要“严格在水后面”的 sample（更远 => depth01 更大）
        if (s > waterDepth01 + eps)
            best = min(best, s);
    }

    // 没找到 behind，就让 thickness=0（ shoreline 会更稳定）
    return (best < 1.0f) ? best : waterDepth01;
}

// 额外：边缘覆盖率（强烈推荐）
float WaterVisibleFrac(int2 pix, float waterDepth01)
{
    int behind = 0;
    const float eps = 1e-4f;

    [unroll] for (int i = 0; i < MSAA_SAMPLES; ++i)
    {
        float s = g_SceneDepth.Load(pix, i);
        if (s > waterDepth01 + eps) behind++;
    }
    return behind / (float)MSAA_SAMPLES;
}
#else
Texture2D<float> g_SceneDepth : register(t41);

float LoadSceneDepth_Min(int2 pix)
{
    return g_SceneDepth.Load(int3(pix, 0));
}

float LoadSceneDepth_BehindWater(int2 pix, float waterDepth01)
{
    
    float s = LoadSceneDepth_Min(pix);
    const float eps = 1e-4f;

    return (s > waterDepth01 + eps) ? s : waterDepth01; // 找不到就返回水自身深度 => thickness=0
}
#endif

// IBL
TextureCube g_Prefilter : register(t31);
Texture2D g_BRDFLUT : register(t32);

// ===== Samplers =====
SamplerState sampClamp : register(s0);
SamplerState sWrap : register(s1);
SamplerState sIBL : register(s2);

// ===== Constant Buffers =====
cbuffer WATER_PS_CB : register(b6)
{
    float4x4 g_InvProj;
    float3 g_EyePosW;
    float g_Time;
   
    
    
    float3 g_AbsorptionRGB;
    float g_RefractStrength;

    float2 g_TexelSize; // 1/w, 1/h
    float2 pad0;

    float g_Roughness;
    float g_FoamFadeDist;
    float g_EnvIntensityWater;
    float g_FoamStrength;

    float2 g_N0Tiling;
    float2 g_N0Speed;
    float2 g_N1Tiling;
    float2 g_N1Speed;
    
    float g_NearZ;
    float g_FarZ;
    float2 g_PadNF;
};

cbuffer IBL_CB : register(b8)
{
    float PrefilterMaxMip;
    float EnvIntensity;
    float2 pad;
};

// ===== PS Input =====
struct PS_IN
{
    float4 posH : SV_POSITION;
    float3 posW : TEXCOORD0;
    float3 posV : TEXCOORD1;
    float3 normalW : TEXCOORD2;
    float3 tangentW : TEXCOORD3;
    float3 bitanW : TEXCOORD4;
    float2 uv : TEXCOORD5;
};

// ===== Helpers =====
float3 UnpackNormal(float4 n)
{
    return normalize(n.xyz * 2 - 1);
}

float3 FresnelSchlick(float NoV, float3 F0)
{
    return F0 + (1 - F0) * pow(1 - NoV, 5);
}

float3 ReconstructViewPos(float2 uv, float depth01)
{
    float4 ndc = float4(uv * 2 - 1, depth01, 1);
    ndc.y = -ndc.y;
    float4 v = mul(ndc, g_InvProj); // ★因为 g_InvProj 实际是 invProjT
    return v.xyz / v.w;
}
float LinearizeViewZ(float depth01)
{
    // viewZ = zn*zf / (zf - depth*(zf-zn))
    return (g_NearZ * g_FarZ) / (g_FarZ - depth01 * (g_FarZ - g_NearZ));
}

float Depth01ToViewZ(float depth01, float nearZ, float farZ)
{
    // D3D depth range [0,1], LH, standard perspective
    return (nearZ * farZ) / (farZ - depth01 * (farZ - nearZ));
}
// ===== Main =====
float4 main(PS_IN pin) : SV_Target
{
    float2 pixf = pin.posH.xy;
    int2   pix  = int2(pixf);

    // 用 float 的屏幕坐标算 UV（比 int2 更平滑）
    float2 suv = (pixf + 0.5f) * g_TexelSize;

    float waterDepth01 = pin.posH.z;   // 水自身 depth01 (0..1)
    float waterZ       = pin.posV.z;   // 水自身 viewZ (LH: 正)

    const float depthBias = 0.02f;     // 2cm(按你世界单位调)

    // ------------------------------------------------------------
    // 1) 遮挡：水在最近表面后面就不画
    //    MSAA 用覆盖率更柔顺；非 MSAA 用 min depth
    // ------------------------------------------------------------
    float visibleFrac = 1.0f;

#if WATER_MSAA
    visibleFrac = WaterVisibleFrac(pix, waterDepth01);
    if (visibleFrac <= 0.0f)
        return float4(0, 0, 0, 0);
#else
    float sceneDepth01Min = LoadSceneDepth_Min(pix);
    float sceneZMin       = LinearizeViewZ(sceneDepth01Min);

    // 水在最近表面后面（被模型/地形挡住）
    if (waterZ >= sceneZMin - depthBias)
        return float4(0, 0, 0, 0);
#endif

    // ------------------------------------------------------------
    // 2) thickness + depthfade：用“水后面的深度”算厚度
    // ------------------------------------------------------------
    float sceneDepth01Behind = LoadSceneDepth_BehindWater(pix, waterDepth01);
    float sceneZBehind       = LinearizeViewZ(sceneDepth01Behind);

    float thickness = max((sceneZBehind - waterZ) - depthBias, 0.0f);

    float edgeFadeDist = max(1e-3f, g_FoamFadeDist); // 也可以单独加参数
    float fade = saturate(thickness / edgeFadeDist);
    fade = fade * fade * (3.0f - 2.0f * fade); // smoothstep

    // ------------------------------------------------------------
    // 3) 原本水着色：法线/折射/反射
    // ------------------------------------------------------------
    float2 uv0 = pin.uv * g_N0Tiling + g_Time * g_N0Speed;
    float2 uv1 = pin.uv * g_N1Tiling + g_Time * g_N1Speed;

    float3 n0 = UnpackNormal(g_Normal0.Sample(sWrap, uv0));
    float3 n1 = UnpackNormal(g_Normal1.Sample(sWrap, uv1));
    float3 Nt = normalize(float3(n0.xy + n1.xy, n0.z * n1.z));

    float3 T = normalize(pin.tangentW);
    float3 B = normalize(pin.bitanW);
    float3 N = normalize(pin.normalW);
    float3x3 TBN = float3x3(T, B, N);
    float3 Nw = normalize(mul(Nt, TBN));

    float3 Vw = normalize(g_EyePosW - pin.posW);
    float NoV = saturate(dot(Nw, Vw));

    // --- refraction ---
    float2 distort = Nw.xz * g_RefractStrength * (1.0f - NoV);
    float3 refr = g_SceneColor.Sample(sampClamp, suv + distort).rgb;

    // absorption 用 thickness
    float3 trans = exp(-g_AbsorptionRGB * max(thickness, 0.02f));
    refr *= trans;

    // --- reflection (IBL) ---
    float3 F0 = float3(0.02, 0.02, 0.02);
    float3 F  = FresnelSchlick(NoV, F0);

    float3 Rw  = reflect(-Vw, Nw);
    float  mip = saturate(g_Roughness) * PrefilterMaxMip;

    float3 pre  = g_Prefilter.SampleLevel(sIBL, normalize(Rw), mip).rgb;
    float2 brdf = g_BRDFLUT.Sample(sampClamp, float2(NoV, saturate(g_Roughness))).rg;
    float3 refl = pre * (F * brdf.x + brdf.y) * g_EnvIntensityWater;

    float3 col = lerp(refr, refl, F);

    // ------------------------------------------------------------
    // 4) 泡沫：不要 col +=（会白边），改用 lerp 到泡沫色
    //    泡沫贴边最强 => (1 - fade)
    // ------------------------------------------------------------
    float foam = saturate((1.0f - fade) * g_FoamStrength);
    float3 foamCol = float3(0.9f, 0.9f, 0.9f);
    col = lerp(col, foamCol, foam);

    // ------------------------------------------------------------
    // 5) alpha：Fresnel * depthfade * MSAA覆盖率
    // ------------------------------------------------------------
    float fres = saturate(1.0f - NoV);
    float baseAlpha = lerp(0.35f, 0.85f, fres);

    float alpha = baseAlpha * fade * visibleFrac;

    return float4(col, alpha);
}