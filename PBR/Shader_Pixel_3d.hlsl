/*

  3D drawing pixel shader (PBR 版示例)

*/

// ========== 常量缓冲（沿用你的结构，尽量不改 C++） ==========

cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color; // baseColor 的整体系数
};

cbuffer PS_CONSTANT_BUFFER : register(b1)
{
    float4 ambient_color; // 环境光颜色
    float padAmbient;
};

cbuffer PS_CONSTANT_BUFFER : register(b2)
{
    float4 directional_world_vector; // xyz: 光方向(你原来的)
    float4 directional_color; // rgb: 方向光颜色
};

cbuffer PS_CONSTANT_BUFFER : register(b3)
{
    float3 eye_PosW; // 摄像机位置
    float metallic;
    float roughness;
    float ao; 
    float2 padPBR; 
};

struct PointLight
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_CONSTANT_BUFFER : register(b4)
{
    PointLight point_light[4];
    int point_light_count;
    float3 padpoint; // 对齐用
};

cbuffer PS_CONSTANT_BUFFER : register(b5)
{
    float NormalFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float AoFactor;
};

cbuffer PS_SHADOW_BUFFER : register(b7)
{
    float4x4 mtxLightVP; // 和 ShadowPass 用同一个 lightVP（注意：按你项目习惯做Transpose后上传）
    float2 shadowMapSize; // (w, h)
    float shadowBias; // 0.0003~0.002 起步
    float normalBias; // 0.001~0.01 视世界单位
};

cbuffer PS_IBL_CB : register(b8)
{
    float g_PrefilterMaxMip; // mipCount - 1，例如 8/9
    float g_EnvIntensity; // 环境强度，1.0 起步
    float2 padIBL;
};

// ========== 输入结构：接收 TBN 所需的数据 ==========
struct PS_IN
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION0;
    float3 normalW : NORMAL0;
    float3 tangentW : TANGENT0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};


// ========== 贴图 ==========
Texture2D g_AlbedoMap : register(t0); // 原来的 tex
Texture2D g_NormalMap : register(t1); // 法线贴图
Texture2D g_MetallicMap : register(t2);
Texture2D g_RoughnessMap : register(t3);

SamplerState samp : register(s0);
// ===== ShadowMap（方向光）=====
Texture2D<float> g_ShadowMap : register(t20);


SamplerComparisonState g_ShadowCmp : register(s1);


// ===== IBL =====

TextureCube g_IrradianceMap : register(t30);
TextureCube g_PrefilterEnvMap : register(t31);
Texture2D g_BRDF_LUT : register(t32);

SamplerState sampClamp : register(s2);

// ========== PBR 辅助函数 ==========
static const float PI = 3.14159265f;

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-4f);
}

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);

    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}


// ========== 从法线贴图 + TBN 得到世界空间法线 ==========
float3 GetNormalFromMap(PS_IN pin)
{
    
    // 几何法线 & 切线（世界空间）
    float3 N = normalize(pin.normalW);
    float3 T = normalize(pin.tangentW);

    // 正交化切线，避免插值误差
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T);

    // 切线空间 normal：[0,1] → [-1,1]
    float3 normalTS = g_NormalMap.Sample(samp, pin.uv).xyz * 2.0f - 1.0f;

    // 如果发现凸凹反了，可以启用这一句：
    // normalTS.y = -normalTS.y;

    // NormalFactor：控制 normal map 强度（0=不用贴图，1=完全按贴图）
    normalTS = normalize(lerp(float3(0, 0, 1), normalTS, NormalFactor));

    // 切线空间 → 世界空间（手写 TBN，避免 mul 行列坑）
    float3 normalWS = normalize(
        normalTS.x * T +
        normalTS.y * B +
        normalTS.z * N
    );

    return normalWS;
}

float ShadowPCF(float3 posW, float3 N, float3 L)
{
    float3 biasedPosW = posW + N * normalBias;

    float4 spH = mul(float4(biasedPosW, 1.0f), mtxLightVP);
    float3 sp = spH.xyz / max(spH.w, 1e-6f);

    float2 uv;
    uv.x = sp.x * 0.5f + 0.5f;
    uv.y = -sp.y * 0.5f + 0.5f;

    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        return 1.0f;
    if (sp.z < 0 || sp.z > 1)
        return 1.0f;

    float ndotl = saturate(dot(N, L));
    float bias = shadowBias + normalBias * (1.0f - ndotl);

    float z = sp.z - bias;

    float2 texel = 1.0f / shadowMapSize;

    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; y++)
    [unroll]
        for (int x = -1; x <= 1; x++)
            sum += g_ShadowMap.SampleCmpLevelZero(g_ShadowCmp, uv + float2(x, y) * texel, z);

    return sum / 9.0f;
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 oneMinusR = float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness);
    return F0 + (max(oneMinusR, F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 ComputeIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness, float ao, float3 F0)
{
    float NdotV = saturate(dot(N, V));
    float3 R = reflect(-V, N);

    float3 kS = FresnelSchlickRoughness(NdotV, F0, roughness);
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    // diffuse IBL
    float diffuseScale = 1.0f;
    float3 irradiance = g_IrradianceMap.Sample(sampClamp, N).rgb;
    float3 diffuseIBL = irradiance * albedo / PI;
   

    // specular IBL
    float lod = g_PrefilterMaxMip; //(roughness * roughness) * g_PrefilterMaxMip;
    float3 prefiltered = g_PrefilterEnvMap.SampleLevel(sampClamp, R, lod).rgb;
    float specScale = 0.2f; // 先从 0.2~0.5 试，专治“高光炸”
    
    float2 brdf = g_BRDF_LUT.SampleLevel(sampClamp, float2(NdotV, roughness), 0).rg;
    float3 specIBL = prefiltered * (kS * brdf.x + brdf.y) * specScale;
    
    float grazing = pow(1.0f - NdotV, 4.0f); // 越靠边越大
    float rimDamp = lerp(1.0f, 0.35f, grazing); // 0.35 越小白边越弱
    specIBL *= rimDamp;
    
    return (kD * diffuseIBL + specIBL) * ao * g_EnvIntensity;
}

float3 ToneMapACES
    (
    float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}




// ========== PBR 主体 ==========
float4 main(PS_IN pin) : SV_TARGET
{
    //return float4(pin.color.rgb, 1);
    // 1) 反照率（albedo）：贴图 * 顶点色 * 材质整体系数
    float4 texSample = g_AlbedoMap.SampleLevel(samp, pin.uv, 0);
    float3 albedo = texSample.rgb * diffuse_color.rgb;

    
    // 2) 世界空间法线 & 视线方向（和地面一样）
    float3 N = GetNormalFromMap(pin);
    float3 V = normalize(eye_PosW - pin.posW);
    
    
    // 3) 从贴图和因子得到 metallic / roughness / ao
    float metallicTex = g_MetallicMap.Sample(samp, pin.uv).r;
    float roughnessTex = g_RoughnessMap.Sample(samp, pin.uv).r;

    // 旧的 cbuffer3.metallic / roughness 先当成“基础值”，再乘贴图和因子
    float metallic_ = saturate(metallicTex * MetallicFactor);
    float roughness_ = saturate(roughnessTex * RoughnessFactor);
    roughness_ = max(roughness_, 0.2f); // 给 roughness 一个下限，避免高光爆闪

    // AO：用 cbuffer3 的 ao 再乘一个因子（暂时没有 AO 贴图就这样用）
    float ao_ = saturate(ao * AoFactor);

    // 4) F0：非金属 ~0.04，金属用 albedo
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic_);

    float3 Lo = 0.0f;

    // ========== 5) 方向光（和地面一样的 PBR） ==========
    {
        float3 L = normalize(-directional_world_vector.xyz); // 如果光在背面，可以试试去掉负号
        float3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0f);
        float NdotV = max(dot(N, V), 0.0f);

        if (NdotL > 0.0f)
        {
            float shadow = ShadowPCF(pin.posW, N, L);
            
            float D = DistributionGGX(N, H, roughness_);
            float G = GeometrySmith(N, V, L, roughness_);
            float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

            float3 numerator = D * G * F;
            float denom = 4.0f * max(NdotV, 0.0f) * NdotL + 1e-4f;
            float3 specular = numerator / denom;
            float grazing = pow(1.0f - saturate(dot(N, V)), 4.0f);
            specular *= lerp(1.0f, 0.35f, grazing);
            
            
            float3 kS = F;
            float3 kD = (1.0f - kS) * (1.0f - metallic_);
            float3 diffuse = albedo / PI;
            float3 radiance = directional_color.rgb;

            Lo += ((kD * diffuse + specular) * radiance * NdotL) * shadow;
        }
    }

    // ========== 6) 点光源（沿用你原来的 range 衰减，只改成用 roughness_ / metallic_） ==========
    [unroll]
    for (int i = 0; i < point_light_count; ++i)
    {
        float3 Lvec = point_light[i].posW - pin.posW;
        float dist = length(Lvec);
        float3 L = Lvec / max(dist, 1e-4f);

        // 你的 range 衰减
        float A = pow(max(1.0f - dist / point_light[i].range, 0.0f), 2.0f);

        float3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0f);
        float NdotV = max(dot(N, V), 0.0f);

        if (NdotL > 0.0f && A > 0.0f)
        {
            float D = DistributionGGX(N, H, roughness_);
            float G = GeometrySmith(N, V, L, roughness_);
            float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

            float3 numerator = D * G * F;
            float denom = 4.0f * max(NdotV, 0.0f) * NdotL + 1e-4f;
            float3 specular = numerator / denom;

            float3 kS = F;
            float3 kD = (1.0f - kS) * (1.0f - metallic_);
            float3 diffuse = albedo / PI;
            float3 radiance = point_light[i].color.rgb * A;
            
            Lo += (kD * diffuse + specular) * radiance * NdotL;
        }
    }

    // ========== 7) 环境光（简易版 IBL 替身） ==========
    float3 ambientIBL = ComputeIBL(N, V, albedo, metallic_, roughness_, ao_, F0);

// 可选：保底一点点旧 ambient，防止你 IBL 资源还没绑好时全黑
// ambientIBL += ambient_color.rgb * albedo * ao_ * 0.1f;
    
    float alpha = texSample.a * diffuse_color.a;
    //clip(alpha - 0.02);
    
    
    float3 color = ambientIBL + Lo;

// 曝光：越大越亮。先从 1.0 开始
    float exposure = 1.0f;
    color *= exposure;

// ★ 关键：tone map 压高光，避免爆白
    color = ToneMapACES(color);
    color = pow(saturate(color), 1.0f / 2.2f);
    //return float4(ao_.xxx, 1);
    return float4(color, alpha);
}
