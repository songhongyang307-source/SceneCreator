/*==============================================================================

  unlit pixel shader[shader_pixel_unlit.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/


cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};

struct PS_IN
{
    float4 posH : SV_POSITION; //system value position
    float2 uv : TEXCOORD0;
};


Texture2D tex : register(t0); //texture(only one texture can be shown)
SamplerState samp : register(s0); //texture  sample(only one picture can be shown)


float4 main(PS_IN pi) : SV_TARGET
{
    float4(pi.uv, 0, 1);
    return tex.Sample(samp, pi.uv) * diffuse_color;
   
    
}