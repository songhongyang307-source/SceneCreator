/*==============================================================================

  Billboard drawing vertex shader[ [shader_vertex_billboard.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バファ


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
    float4x4 mtxProj; //矩阵投影 metrix projection
};

cbuffer VS_CONSTANT_BUFFER : register(b3)
{
    float2 scale;
    float2 translation; //矩阵投影 metrix projection
};

struct VS_IN
{
    float4 posL : POSITION0; //localposition
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};



struct VS_OUT
{
    float4 posH : SV_POSITION; //system value position
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};


//=============================================================================
// 頂点シェーダー
//=============================================================================


VS_OUT main(VS_IN vi) //: SV_POSITION
{
    VS_OUT vo;
   
    float4x4 mtxWV = mul(mtxWorld, mtxView);
    float4x4 mtxWVP = mul(mtxWV, mtxProj);
    vo.posH = mul(vi.posL, mtxWVP);
    
    
    vo.color = vi.color;
    vo.uv = vi.uv * scale + translation;
    
    return vo;
}
