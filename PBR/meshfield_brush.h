/*==============================================================================

   MESHFIELD_BRUSH_H[meshfield_brush.h]
														 Author : Theshy
														 Date   : 2025/12/06
--------------------------------------------------------------------------------

==============================================================================*/


#include "DirectXMath.h"

#ifndef MESHFIELD_BRUSH_H
#define MESHFIELD_BRUSH_H

enum BrushLayer
{
    BRUSH_R = 0, // 草
    BRUSH_G = 1, // 泥
    BRUSH_B = 2, // 石
    BRUSH_A = 3  // 路/雪
};

extern BrushLayer g_CurrentBrush ;

void UpdateBrushInput();
void PaintControlMapAt(const DirectX:: XMFLOAT3& hitPos);
#endif // !MESHFIELD_BRUSH_H