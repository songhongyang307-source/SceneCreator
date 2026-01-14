/*==============================================================================

   3D Shader (Unlighting)[shader3d_unlit.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SHADER3D_UNLIT_H
#define	SHADER3D_UNLIT_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader3dUnlit_Initialize();
void Shader3dUnlit_Finalize();

void Shader3dUnlit_SetWorldMatrix(const DirectX::XMMATRIX& matrix);


void Shader3dUnlit_SetColor(const DirectX::XMFLOAT4& color);
void Shader3dUnlit_Begin();

#endif // SHADER3D_UNLIT_H
