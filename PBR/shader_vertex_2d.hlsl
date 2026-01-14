/*==============================================================================

   2D�`��p���_�V�F�[�_�[ [shader_vertex_2d.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バファ
cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 proj; //矩阵投影 metrix projection
};

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 world; 
};


struct VS_IN
{
    float4 posL : POSITION0; //localposition
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};
struct VS_OUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

//=============================================================================
// 頂点シェーダー
//=============================================================================
//从cpu读取情报时。，读取postion0和color0，这两个不是变量
VS_OUT main(VS_IN vi) //: SV_POSITION
{
    VS_OUT vo;
    
    
    
    
    
    //坐标变换、对于坐标的处理要放在这行之上
    float4x4 mtx = mul(world, proj);
    
    vo.posH = mul(vi.posL, mtx);
    
    
    vo.color = vi.color;
    vo.uv = vi.uv; //uv is sent to pixel shader
    
	return vo;
}
