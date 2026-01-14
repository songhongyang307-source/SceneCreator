 /*==============================================================================

   3d drawing vertex field shader[ [shader_vertex_field.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/05/15
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






struct VS_IN
{
    float4 posL : POSITION0; //localposition
    float3 normalL : NORMAL0;
    float3 tangentL : TANGENT0;
    float4 blend : COLOR0;
    float2 uv : TEXCOORD0;
};



struct VS_OUT
{
    float4 posH : SV_POSITION; //system value position
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float3 tangentW : TANGENT0;
    float4 blend: COLOR0;
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
    //float4 posW = mul(vi.posL, world); //local to  world
   // float4 posWV = mul(posW, view); //view
   // vo.posH = mul(posWV, proj); //perspective
   
    float4 posL = float4(vi.posL.xyz, 1.0f);

    float4x4 mtxWV = mul(mtxWorld, mtxView);
    float4x4 mtxWVP = mul(mtxWV, mtxProj);
    vo.posH = mul(posL, mtxWVP);

    vo.posW = float4(mul(posL, mtxWorld).xyz, 1.0f);

    // World 的 3×3 部分
    float3x3 world3x3 = (float3x3) mtxWorld;

    // normal
    float3 Nw = normalize(mul(vi.normalL, world3x3));

    // 用一个“向上向量”构造切线（与 N 不平行即可）
    float3 up = (abs(Nw.y) < 0.999f) ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 Tw = normalize(cross(up, Nw)); // T 和 N 正交
    Tw = normalize(Tw - dot(Tw, Nw) * Nw); // 再正交一次，保险

    vo.normalW = float4(Nw, 0.0f);
    vo.tangentW = Tw;

    vo.blend = vi.blend;
    vo.uv = vi.uv;

    return vo;
}
