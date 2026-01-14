cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 mtxWorld;
};
cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 mtxView;
};
cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 mtxProj;
};
cbuffer CB_Wind : register(b7)
{
    float3 g_WindDirW; // 世界风方向（单位向量）
    float g_WindStrength; // 总强度（0~1）

    float g_WindSpeed; // 时间速度（0.5~3）
    float g_WindFreq; // 空间频率（0.05~0.3）
    float g_GustFreq; // 阵风频率（0.2~1.5）
    float g_Time; // 秒

    float g_Flutter; // 叶子抖动（0~1）
    float3 g_Pad;
};
struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 color : COLOR0; // ★加上，避免你复用 layout 时 CreateInputLayout 失败
    float2 uv : TEXCOORD0;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION0;
    float3 normalW : NORMAL0;
    float3 tangentW : TANGENT0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};


float Hash11(float n)
{
    return frac(sin(n) * 43758.5453);
}
float Hash12(float2 p)
{
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float3 ApplyWindOffset(float3 worldPos, float3 normalW, float weight)
{
    weight = saturate(weight);
    if (weight <= 1e-4)
        return worldPos;

    float3 windDir = normalize(g_WindDirW);

    // 低频摆动（树叶/草整体随风）
    float phase = dot(worldPos.xz, windDir.xz) * g_WindFreq + g_Time * g_WindSpeed;
    float sway = sin(phase) * 0.5 + sin(phase * 0.7) * 0.5; // 叠两层更自然

    // 阵风（整体强度起伏）
    float gust = 0.6 + 0.4 * sin(g_Time * g_GustFreq + Hash12(worldPos.xz) * 6.2831);

    float amp = g_WindStrength * gust * weight;

    // 沿风方向推
    float3 offset = windDir * (sway * amp);

    // 高频 flutter（只给叶子更明显，沿法线/切线抖）
    float flutter = sin(phase * 3.0 + Hash12(worldPos.xz) * 10.0) * (g_Flutter * amp);
    offset += normalW * flutter;

    return worldPos + offset;
}



VS_OUT main(VS_IN v)
{
    VS_OUT o;

    // local -> world
    float4 posW4 = mul(float4(v.pos, 1.0), mtxWorld);

    float3x3 W3 = (float3x3) mtxWorld;

    float3 Nw = normalize(mul(v.normal, W3));
    float3 Tw = mul(v.tangent, W3);
    if (dot(Tw, Tw) < 1e-6)
        Tw = float3(1, 0, 0);
    Tw = normalize(Tw - Nw * dot(Tw, Nw));

    // ===== 这里加风（在算 posH 之前）=====
    float windWeight = v.color.r; // 约定 R 通道控制风
    posW4.xyz = ApplyWindOffset(posW4.xyz, Nw, windWeight);
    // =====================================

    o.posH = mul(mul(posW4, mtxView), mtxProj);

    o.posW = posW4.xyz;

    o.normalW = Nw;
    o.tangentW = Tw;
    o.uv = v.uv;

    o.color = v.color;

    return o;
}
