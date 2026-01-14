/*==============================================================================

   3d CUBE visualize[cube.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef CUBE_H
#define CUBE_H


#include <d3d11.h>
#include <DirectXMath.h>
#include "collision.h"




void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Cube_Draw(int Texid,const DirectX::XMMATRIX& mtxworld);

void Cube_Draw(int diffuse_tex_id, int metallic_tex_id, int roughness_tex_id, int normal_tex_id, const DirectX::XMMATRIX& mtxworld);

void Cube_Finalize(void);

AABB Cube_GetAABB(const DirectX::XMFLOAT3 position);
#endif  // CUBE_H