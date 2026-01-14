/*==============================================================================

   Billboard  shader  [shader_billboard.h]
														 Author : Youhei Sato
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/




#ifndef SHADER_BillBOARD_H
#define	SHADER_BillBOARD_H

#include <d3d11.h>
#include <DirectXMath.h>


bool ShaderBillboard_Initialize();
void ShaderBillboard_Finalize();
	 
void ShaderBillboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
//void ShaderBillboard_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);
//void ShaderBillboard_SetViewMatrix(const DirectX::XMMATRIX& matrix);
	
void ShaderBillboard_SetColor(const DirectX::XMFLOAT4& color);
void ShaderBillboard_Begin();


struct UVParameter
{
	DirectX::XMFLOAT2 scale;
	DirectX::XMFLOAT2 translation;
};

void ShaderBillboard_SetUVParameter(const UVParameter& parameter);
#endif // SHADER_BillBOARD_H
