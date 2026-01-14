/*==============================================================================

   Shadow  shader[shader_shadow.h]
														 Author : the shy
														 Date   : 2025/12/17
--------------------------------------------------------------------------------

==============================================================================*/



#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

bool ShadowShader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShadowShader_Finalize();

void ShadowShader_Begin();     // 会设置 Blend/Depth/RS（适合阴影）
void ShadowShader_End();       // 还原状态（nullptr）

void ShadowShader_SetWVPMatrix(const DirectX::XMMATRIX& wvp);
void ShadowShader_SetShadowColor(const DirectX::XMFLOAT4& rgba);