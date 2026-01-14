/*==============================================================================

   field  shader[ [shader3d.h]
														 Author : Youhei Sato
														 Date   : 2025/09/26
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER_FIELD_H
#define	SHADER_FIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

struct FieldMaterial
{
	float NormalFactor;
	float MetallicFactor;
	float RoughnessFactor;
	float AoFactor;

};

struct UVTiling
{
	DirectX::XMFLOAT2 uvTiling;
	DirectX::XMFLOAT2 uvTilingPad;

};


bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShaderField_Finalize();

void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
//void ShaderField_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);
//void ShaderField_SetViewMatrix(const DirectX::XMMATRIX& matrix);

void ShaderField_SetColor(const DirectX::XMFLOAT4& color);
void ShaderField_Begin();

void ShaderField_SetMaterialParam(float normalfactor, float metallicfactor, float roughnessfactor, float aofactor);

void ShaderField_SetTiling(float u = 1.0f, float v = 1.0f);
#endif // SHADER_FIELD_H
