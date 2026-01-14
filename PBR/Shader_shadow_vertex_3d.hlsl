

/*==============================================================================

   shadow vertex [Shader_shadow_vertex_3d.hlsl]
														 Author : the shy
														 Date   : 2025/12/25
--------------------------------------------------------------------------------

==============================================================================*/



cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 mtxWVP;
};

struct VS_IN
{
    float3 posL : POSITION0;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
};

VS_OUT main(VS_IN vin)
{
    VS_OUT o;
    o.posH = mul(float4(vin.posL, 1.0f), mtxWVP);
    return o;
}