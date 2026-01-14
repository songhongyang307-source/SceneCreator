/*

3D drawing  pixel field shader




*/


cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};
cbuffer PS_CONSTANT_BUFFER : register(b1)
{
    float4 ambient_color; //矩阵投影 metrix projection
};


cbuffer PS_CONSTANT_BUFFER : register(b2)
{
    float4 directional_world_vector;
    float4 directional_color = { 1.0f, 1.0f, 1.0f, 1.0f }; //矩阵投影 metrix projection
   
    
};

cbuffer PS_CONSTANT_BUFFER : register(b3)
{
    float3 eye_PosW;
    float metallic;
    float roughness;
    float ao=1.0f;
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
    const int point_light_count = 4;
    
    float3 padpoint;
      
};

cbuffer PS_CONSTANT_BUFFER : register(b5)
{
    float NormalFactor;
    float MetallicFactor;
    float RoughnessFactor ;
    float AoFactor ;
};


cbuffer PS_CONSTANT_BUFFER : register(b6)
{
    float2 uvTiling;
    float2 uvTilingPad;
};

cbuffer SHADOW_CB : register(b7)
{
    float4x4 g_mtxLightVP; // C++ 里 transpose 后写入
    float2 g_shadowMapSize; // (w,h)
    float g_shadowBias;
    float g_normalBias;
};

struct PS_IN
{
    float4 posH : SV_POSITION; //system value position
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float3 tangentW : TANGENT0;
    float4 blend : COLOR0;
    float2 uv : TEXCOORD0;
    
};


//混合texture ，最大四张 ，可以使用数组定义
Texture2D g_ControlMap : register(t0);

Texture2D g_AlbedoMap0 : register(t1);
Texture2D g_NormalMap0 : register(t2);
Texture2D g_MetallicMap0 : register(t3);
Texture2D g_RoughnessMap0 : register(t4);

Texture2D g_AlbedoMap1 : register(t5);
Texture2D g_NormalMap1 : register(t6);
Texture2D g_MetallicMap1 : register(t7);
Texture2D g_RoughnessMap1 : register(t8);

Texture2D g_AlbedoMap2 : register(t9);
Texture2D g_NormalMap2 : register(t10);
Texture2D g_MetallicMap2 : register(t11);
Texture2D g_RoughnessMap2 : register(t12);

Texture2D g_AlbedoMap3 : register(t13);
Texture2D g_NormalMap3 : register(t14);
Texture2D g_MetallicMap3 : register(t15);
Texture2D g_RoughnessMap3 : register(t16);

Texture2D<float> g_ShadowMap : register(t20);
SamplerComparisonState g_ShadowSamp : register(s1);
//Texture2D g_SpecularMap : register(t4);


SamplerState samp : register(s0);

// ---------------- PBR 工具函数 ----------------
static const float PI = 3.14159265f;

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float3 N, float3 H, float roughness)//在法线 N 周围，朝向 H 的那些微小镜面有多少？
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-4f);
}

float GeometrySchlickGGX(float NdotV, float k)    //k=(roughness+1)^2 / 8
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
float CalcShadowFactor(float3 posW, float3 N, float3 L)
{
    float4 posL = mul(float4(posW, 1.0f), g_mtxLightVP);

    float3 ndc = posL.xyz / max(posL.w, 1e-6f);

    float2 uv;
    uv.x = ndc.x * 0.5f + 0.5f;
    uv.y = -ndc.y * 0.5f + 0.5f; // 如果发现上下颠倒，把前面的负号去掉

    float depth = ndc.z;

    // 超出范围直接当作不在阴影里
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || depth < 0 || depth > 1)
        return 1.0f;

    float ndotl = saturate(dot(N, L));
    float bias = g_shadowBias + g_normalBias * (1.0f - ndotl);

    float2 texel = 1.0f / g_shadowMapSize;

    float s = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 uv2 = uv + float2(x, y) * texel;
            s += g_ShadowMap.SampleCmpLevelZero(g_ShadowSamp, uv2, depth - bias);
        }
    }
    return s / 9.0f; // 1=亮 0=阴影
}

float4 main(PS_IN pin) : SV_TARGET
{
    float2 tiledUV = pin.uv * uvTiling;

    // =====================================
    // Albedo 混合（和你现在版本一致）
    // =====================================
    float4 mask = g_ControlMap.Sample(samp, pin.uv);
    float total = max(mask.r + mask.g + mask.b + mask.a, 1e-5f);
    float4 w = mask / total;

    float3 a0 = g_AlbedoMap0.Sample(samp, tiledUV).rgb;
    float3 a1 = g_AlbedoMap1.Sample(samp, tiledUV).rgb;
    float3 a2 = g_AlbedoMap2.Sample(samp, tiledUV).rgb;
    float3 a3 = g_AlbedoMap3.Sample(samp, tiledUV).rgb;

    
    float3 albedoBase = a0 * w.r + a1 * w.g + a2 * w.b + a3 * w.a;
    float3 albedo = albedoBase * diffuse_color.rgb;


    // =====================================
    // PBR 参数贴图
    // =====================================
    float m0 = saturate(g_MetallicMap0.Sample(samp, tiledUV).r * MetallicFactor);
    float m1 = saturate(g_MetallicMap1.Sample(samp, tiledUV).r * MetallicFactor);
    float m2 = saturate(g_MetallicMap2.Sample(samp, tiledUV).r * MetallicFactor);
    float m3 = saturate(g_MetallicMap3.Sample(samp, tiledUV).r * MetallicFactor);
   
    float metallicBase = m0 * w.r + m1 * w.g + m2 * w.b + m3 * w.a;
    float3 metallic = metallicBase;
    
    float r0 = max(saturate(g_RoughnessMap0.Sample(samp, tiledUV).r * RoughnessFactor), 0.04f);
    float r1 = max(saturate(g_RoughnessMap1.Sample(samp, tiledUV).r * RoughnessFactor), 0.04f);
    float r2 = max(saturate(g_RoughnessMap2.Sample(samp, tiledUV).r * RoughnessFactor), 0.04f);
    float r3 = max(saturate(g_RoughnessMap3.Sample(samp, tiledUV).r * RoughnessFactor*2), 0.04f);

    float roughnessBase = r0 * w.r + r1 * w.g + r2 * w.b + r3 * w.a;
    float roughness = roughnessBase;
    
    float3 nTS_raw0 = g_NormalMap0.Sample(samp, tiledUV).xyz * 2.0 - 1.0;
    float3 nTS_raw1 = g_NormalMap1.Sample(samp, tiledUV).xyz * 2.0 - 1.0;
    float3 nTS_raw2 = g_NormalMap2.Sample(samp, tiledUV).xyz * 2.0 - 1.0;
    float3 nTS_raw3 = g_NormalMap3.Sample(samp, tiledUV).xyz * 2.0 - 1.0;
    nTS_raw3 *= float3(1, -1, 1);
    
    float3 normalBase = nTS_raw0 * w.r + nTS_raw1 * w.g + nTS_raw2 * w.b + nTS_raw3 * w.a;
    float3 normal = normalBase;
    // =====================================
    // 计算 TBN
    // =====================================
    float3 Nw = normalize(pin.normalW.xyz);
    float3 Tw = normalize(pin.tangentW);
    Tw = normalize(Tw - dot(Tw, Nw) * Nw); // Gram-Schmidt 正交化
    float3 Bw = normalize(cross(Nw, Tw)); // 如出现反向，可试 cross(Tw, Nw)

    float3 nTS_raw = normal;
    float3 nTS = normalize(lerp(float3(0, 0, 1), nTS_raw, NormalFactor));

    float3 N = normalize(
        nTS.x * Tw +
        nTS.y * Bw +
        nTS.z * Nw
    );


    // =====================================
    // 光照方向（注意必须是负方向）
    // =====================================
    float3 L = normalize(-directional_world_vector.xyz);
    float3 V = normalize(eye_PosW - pin.posW.xyz);
    float3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));


    // =====================================
    // Fresnel / GGX / Geometry
    // =====================================
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    float3 numerator = D * G * F;
    float denominator = max(4.0 * NdotL * NdotV, 1e-4f);
    float3 specular = numerator / denominator;
    specular = pow(specular, 1);
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 diffuse = kD * albedo / PI;

    float shadow = CalcShadowFactor(pin.posW.xyz, N, L);

// 方向光受阴影影响
    float3 Lo = ((diffuse + specular) * directional_color.rgb * NdotL) * shadow;

// 建议至少给一点环境光，不然阴影区会死黑
    float3 ambient = ambient_color.rgb * albedo * saturate(ao * AoFactor);

    float3 color = ambient + Lo;
    return float4(color, 1.0f);
    
}
   /*
    float2 tiledUV = pin.uv * uvTiling;
    
    float4 mask = g_ControlMap.Sample(samp, pin.uv);
    
    //make sure total value of rgba of mask is 1
    float total = mask.r + mask.g + mask.b + mask.a;
    total = max(total, 1e-4f);
    float4 w = mask / total;
    
    float3 a0 = g_AlbedoMap0.Sample(samp, tiledUV).rgb;
    float3 a1 = g_AlbedoMap1.Sample(samp, tiledUV).rgb;
    float3 a2 = g_AlbedoMap2.Sample(samp, tiledUV).rgb;
    float3 a3 = g_AlbedoMap3.Sample(samp, tiledUV).rgb;
    
    float3 albedoBase = a0 * w.r + a1 * w.g + a2 * w.b + a3 * w.a;
    
    float3 albedo =albedoBase * diffuse_color.rgb;
    
    
    
    
    float metallic = g_MetallicMap.Sample(samp, tiledUV).r;
    metallic = saturate(metallic * MetallicFactor);
    
    float roughness = g_RoughnessMap.Sample(samp, tiledUV).r;
    roughness =max(saturate(roughness * RoughnessFactor), 0.04f);
   
    float3 Nw = normalize(pin.normalW.xyz);
    float3 Tw = normalize(pin.tangentW);
    Tw = normalize(Tw - dot(Tw, Nw) * Nw);
    float3 Bw = normalize(cross(Nw, Tw));

    float3 nTS_raw = g_NormalMap.Sample(samp, tiledUV).xyz * 2.0f - 1.0f;
// normal 强度：1.0 原样，0.0 完全用几何法线
    float3 nTS = normalize(lerp(float3(0, 0, 1), nTS_raw, NormalFactor));
// 如果发现高低反了，再试 nTS.y = -nTS.y;
    float3 N = normalize(
    nTS.x * Tw +
    nTS.y * Bw +
    nTS.z * Nw
);

    float3 L = normalize(-directional_world_vector.xyz); // 方向光方向（需要的话可以试试去掉负号）
    float3 V = normalize(eye_PosW - pin.posW.xyz); // 世界空间视线方向
    float3 H = normalize(L + V); // 半程向量

    float NdotL = dot(N, L);
    NdotL = saturate(NdotL);
    
    float NdotV = saturate(dot(N, V));
    
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

// 2. 计算 F, D, G
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

// 3. Cook-Torrance 高光
    float3 numerator = D * G * F;
    float denominator = 4.0f * max(NdotV, 1e-4f) * max(NdotL, 1e-4f);
    float3 specular = numerator / denominator;

// 4. 漫反射部分（能量守恒：kD + kS = 1）
    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic); // 金属不参与漫反射
    float3 diffuse = kD * albedo / PI;

// 5. 单个方向光的 Lo
    float3 lightColor = directional_color.rgb;
    
    float3 Lo = (diffuse + specular) * lightColor * NdotL;
    
    // ==== 5. diffuse + specular ====
    float3 ambient = ambient_color.rgb * albedo * saturate(ao * AoFactor);;

    // 建议：先用 ambient + Lo，避免背光区全黑
    float3 color = ambient + Lo;
    
    // color = pow(color, 1.0f / 2.2f);

    return float4(color, 1.0f);
}
*/


    
 