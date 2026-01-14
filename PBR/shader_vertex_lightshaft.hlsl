struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut VS_Fullscreen(uint id : SV_VertexID)
{
    VSOut o;
    float2 p;
    if (id == 0)
        p = float2(-1.0, -1.0);
    else if (id == 1)
        p = float2(-1.0, 3.0);
    else
        p = float2(3.0, -1.0);

    o.pos = float4(p, 0.0, 1.0);

    // 与你 fog/water 的写法保持一致（如果上下颠倒，就改成 (p.y+1)*0.5）
    o.uv = float2((p.x + 1.0) * 0.5, (1.0 - p.y) * 0.5);
    return o;
}
