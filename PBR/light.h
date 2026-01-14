/*==============================================================================

   ライトの設定[light.h]
														 Author : Youhei Sato
														 Date   : 2025/09/30
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef LIGHT_H
#define LIGHT_H

#include <d3d11.h>
#include <DirectXMath.h>

void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Light_SetAmbient(const DirectX::XMFLOAT3& color);
void Light_Finalize(void);

void Light_SetDirectional(const DirectX::XMFLOAT4& world_directional,
	const DirectX::XMFLOAT4& color);

void Light_Set_PBRParams(
	const DirectX::XMFLOAT3& camera_position,
	float metallic,
	float roughness,
	float ao);



/*==============point light process==============*/
void Light_Set_PointLight(
	int n ,
	const DirectX::XMFLOAT3& position,
	float range,
	const DirectX::XMFLOAT3& color
	);


void Light_Set_PointLight_Count(int count);
/*==============point light process==============*/


/*==============spot light process==============*/
/*
void Light_Set_SpotLight(
	int n,
	const DirectX::XMFLOAT3& position,
	float range,
	const DirectX::XMFLOAT3& color,
);

*/


/*==============spot light process==============*/
#endif // !LIGHT_H
