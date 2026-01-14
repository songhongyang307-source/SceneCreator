/*==============================================================================

   MESHFIELD_BRUSH_H[meshfield_brush.cpp]
														 Author : Theshy
														 Date   : 2025/12/06
--------------------------------------------------------------------------------

==============================================================================*/

#include "meshfield_brush.h"
#include "Keylogger.h"
#include "meshfield.h"
#include "direct3d.h"
#include <algorithm> 
#include "DirectXMath.h"
#include "numeric"

inline float Clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

BrushLayer g_CurrentBrush = BRUSH_R;

void UpdateBrushInput()
{
    if (KeyLogger_IsPressed(KK_NUMPAD1)) g_CurrentBrush = BRUSH_R;
    if (KeyLogger_IsPressed(KK_NUMPAD2)) g_CurrentBrush = BRUSH_G;
    if (KeyLogger_IsPressed(KK_NUMPAD3)) g_CurrentBrush = BRUSH_B;
    if (KeyLogger_IsPressed(KK_NUMPAD4)) g_CurrentBrush = BRUSH_A;
}

void PaintControlMapAt(const DirectX::XMFLOAT3& hitPos)
{
    float u, v;
    if (!GroundPosToControlUV(hitPos, u, v)) return;

    int px, py;
    if (!ControlUVToPixel(u, v, px, py)) return;

    const int brushRadius = 8;
    const float strength = 0.4f;

    for (int dy = -brushRadius; dy <= brushRadius; ++dy)
    {
        for (int dx = -brushRadius; dx <= brushRadius; ++dx)
        {
            int sx = px + dx;
            int sy = py + dy;
            if (sx < 0 || sx >= CONTROL_W ||
                sy <0 || sy >= CONTROL_H)continue;

            float dist2 = float(dx * dx + dy * dy);
            float r2 = float(brushRadius * brushRadius);
            if (dist2 > r2) continue;

            float dist = sqrtf(dist2);
            float w = 1.0f - dist / brushRadius;

            ControlPixel& p = g_ControlCPU[sy * CONTROL_W + sx];

            float fr = p.r / 255.0f;
            float fg = p.g / 255.0f;
            float fb = p.b / 255.0f;
            float fa = p.a / 255.0f;

            float add = strength * w;

            switch (g_CurrentBrush)
            {
            case BRUSH_R: fr += add; break;
            case BRUSH_G: fg += add; break;
            case BRUSH_B: fb += add; break;
            case BRUSH_A: fa += add; break;
            }
            const float decay = add * 0.5f;
            if (g_CurrentBrush != BRUSH_R) fr -= decay;
            if (g_CurrentBrush != BRUSH_G) fg -= decay;
            if (g_CurrentBrush != BRUSH_B) fb -= decay;
            if (g_CurrentBrush != BRUSH_A) fa -= decay;
            
            fr = Clamp01(fr);
            fg = Clamp01(fg);
            fb = Clamp01(fb);
            fa = Clamp01(fa);

            // 归一化一下，保证总和不会太大（可选）
            float sum = fr + fg + fb + fa;
            if (sum > 0.0001f)
            {
                fr /= sum;
                fg /= sum;
                fb /= sum;
                fa /= sum;
            }

            // 转回 0~255
            p.r = (uint8_t)(fr * 255.0f);
            p.g = (uint8_t)(fg * 255.0f);
            p.b = (uint8_t)(fb * 255.0f);
            p.a = (uint8_t)(fa * 255.0f);
        }
    }
    // 改完 CPU 数据后，更新 GPU 纹理
    Direct3D_GetContext()->UpdateSubresource(
        g_pControlTex,
        0,
        nullptr,
        g_ControlCPU.data(),
        CONTROL_W * sizeof(ControlPixel),
        0
    );
}

