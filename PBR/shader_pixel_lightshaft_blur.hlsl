Texture2D g_Mask : register(t50);
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

float4 PS_LightShaftBlur(VSOut pin) : SV_Target
{
    float2 uv = pin.uv;

    // 径向采样次数：低分辨率下 32~64 都可
    const int NUM_SAMPLES = 48;

    // 从当前像素沿着 “朝太阳方向” 走
    float2 delta = (uv - g_SunUV) * (g_Density / NUM_SAMPLES);

    float illumDecay = 1.0f;
    float acc = 0.0f;

    float2 coord = uv;

    [unroll]
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        coord -= delta;
        float s = g_Mask.SampleLevel(sampClamp, coord, 0).r;

        acc += s * illumDecay * g_Weight;
        illumDecay *= g_Decay;
    }

    float rays = acc * g_Exposure * g_Intensity;
    return float4(rays, 0, 0, 1); // rayRT 用 R16_FLOAT，取 .r 即可
}
