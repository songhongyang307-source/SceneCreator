/*==============================================================================

  player controller [player.h]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PLAYER_H

#define PLAYER_H

#include <DirectXMath.h>
using namespace DirectX;
#include "collision.h"


void Player_Initialize(const XMFLOAT3& position, const XMFLOAT3& front);

void Player_Finalize();

void Player_Update(double elapsed_time);

void Player_Draw();

const DirectX::XMFLOAT3& Player_GetPosition();

const DirectX::XMFLOAT3& Player_GetFrontVector();

AABB Player_GetAABB();

AABB Player_ConvertPositionToAABB(const DirectX::XMVECTOR& position);

#endif // !PLAYER_H
