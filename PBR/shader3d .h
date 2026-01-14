/*==============================================================================

   3d  shader[ [shader3d.h]
														 Author : Youhei Sato
														 Date   : 2025/09/9
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER3D_H
#define	SHADER3D_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"



void Shader3d_BeginPassOnce();
void Shader3d_BindIBL_Once();
void Shader3d_BeginFrame();
void DBG(const char* fmt, ...);
void Shader3d_InitPBRUtilityTextures();

bool Shader3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader3d_Finalize();

void Shader3d_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
//void Shader3d_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);
//void Shader3d_SetViewMatrix(const DirectX::XMMATRIX& matrix);

void Shader3d_SetColor(const DirectX::XMFLOAT4& color);
void Shader3d_Begin();
void Shader3d_SetPBRMaterialParams(const PBRMaterial& mat);
void Shader3d_SetIBLParams(float prefilterMaxMip, float envIntensity);


ID3D11ShaderResourceView* Shader3d_GetIrradianceSRV();
ID3D11ShaderResourceView* Shader3d_GetPrefilterSRV();
ID3D11ShaderResourceView* Shader3d_GetBRDFLUTSRV();
float Shader3d_GetPrefilterMaxMip();
ID3D11Buffer* Shader3d_GetIBLConstantBuffer(); // b8

void Shader3d_SetWindParams(const DirectX::XMFLOAT3& dirW,
	float strength, float speed, float freq,
	float gustFreq, float time, float flutter);


#endif // SHADER3D_H
