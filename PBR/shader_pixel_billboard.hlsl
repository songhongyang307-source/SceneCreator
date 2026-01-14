/*==============================================================================

  Billboard drawing pixel shader[shader_pixel_billboard.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/


struct PS_IN
{
    float4 posH : SV_POSITION; //system value position
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};


Texture2D tex; //texture(only one texture can be shown)
SamplerState samp; //texture  sample(only one picture can be shown)


float4 main(PS_IN pi) : SV_TARGET
{
    
    // A * B 
    // A.R * B.R     A.G * B.G  A.B*B.G  A.A*B.A
    return tex.Sample(samp, pi.uv) * pi.color; //返回四位数值，uv的颜色
}