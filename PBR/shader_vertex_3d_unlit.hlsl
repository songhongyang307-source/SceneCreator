 /*==============================================================================

   3D Drawing Vertex Shader (Unlit)[ [Shader_vertex_3d_unlit.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バファ


cbuffer CBWorld : register(b0)
{
    float4x4 mtxWorld;
};
cbuffer CBView : register(b1)
{
    float4x4 mtxView;
};
cbuffer CBProj : register(b2)
{
    float4x4 mtxProj;
};
struct VS_IN
{
    float3 posL : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};
struct VS_OUT
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
};
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    float4x4 mtxWV = mul(mtxWorld, mtxView);
    float4x4 mtxWVP = mul(mtxWV, mtxProj);
    vo.posH = mul(float4(vi.posL, 1.0f), mtxWVP);
    vo.uv = vi.uv;
    return vo;
}