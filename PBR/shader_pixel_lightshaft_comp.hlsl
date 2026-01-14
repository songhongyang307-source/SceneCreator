Texture2D g_Rays : register(t51);
SamplerState sampClamp : register(s0);

cbuffer CB_Shaft : register(b7)
{
    float4x4 g_InvProj;
    float2 g_SunUV;
    float g_Intensity;
    float g_SkyDepthEps;

    float3 g_SunDirV;
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

float4 PS_LightShaftComposite(VSOut pin) : SV_Target
{
    float r = g_Rays.SampleLevel(sampClamp, pin.uv, 0).r;

    // 先用白色光柱；想上色就把这里改成 float3(1,0.95,0.85) * r 之类
    float3 col = r.xxx;

    return float4(col, 1.0f); // alpha=1，配合你的 Add blend 最稳定
}
