/*==============================================================================

   BILLBOARD Drawing[billboard.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/

#include <d3d11.h>
#include <DirectXMath.h>



#ifndef BILLBOARD_H
#define BILLBOARD_H

void BillBoard_Initialize();
void BillBoard_Draw(int Texid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2 scale , const DirectX::XMFLOAT2& pivot = {0.0f , 0.0f});

void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view);


void BillBoard_Draw(int Texid,
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT2 scale,
	const DirectX::XMUINT4& tex_cut,
	const DirectX::XMFLOAT2& pivot = { 0.0f , 0.0f });

void BillBoard_Finalize(void);


#endif // !BILLBOARD_H
