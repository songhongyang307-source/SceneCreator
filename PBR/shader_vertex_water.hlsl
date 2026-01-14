
/*shader water vertex hlsl*/

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



struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT; // 你现在输入是 float3，没有 tangent.w 的手性也能用
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;

    float3 posW : TEXCOORD0;
    float3 posV : TEXCOORD1; // 推荐传：厚度 thickness = sceneV.z - waterV.z 更稳
    float3 Nw :   TEXCOORD2;
    float3 Tw :   TEXCOORD3;
    float3 Bw :   TEXCOORD4;
    float2 uv :   TEXCOORD5;
};



float WaveHeight(float3 posW, float time)
{
    // 很简单的示例：两条正弦叠加
    float h = 0.0;
    h += sin(posW.x * 0.35 + time * 1.2) * 0.08;
    h += sin(posW.z * 0.22 + time * 0.9) * 0.06;
    return h;
}

VS_OUT main(VS_IN v)
{
    VS_OUT o;

    // Local -> World
    float4 posW4 = mul(float4(v.pos, 1.0f), mtxWorld);
    float3 posW = posW4.xyz;

    // （可选）顶点位移：只改 posW.y
    // posW.y += WaveHeight(posW, g_Time);

    // ★如果你改了 posW，一定要重建 posW4（否则后面还是用老位置）
    posW4 = float4(posW, 1.0f);

    // World -> View -> Clip
    float4 posV4 = mul(posW4, mtxView);

    o.posW = posW;
    o.posV = posV4.xyz; // ★float3
    o.posH = mul(posV4, mtxProj);

    o.uv = v.uv;

    // ---- TBN ----
    float3x3 W3 = (float3x3) mtxWorld;

    float3 Nw = normalize(mul(v.normal, W3));
    float3 Tw = mul(v.tangent, W3);

    if (dot(Tw, Tw) < 1e-6f)
        Tw = float3(1, 0, 0);

    Tw = normalize(Tw - Nw * dot(Tw, Nw));
    float3 Bw = normalize(cross(Nw, Tw));

    o.Nw = Nw;
    o.Tw = Tw;
    o.Bw = Bw;

    return o;
}