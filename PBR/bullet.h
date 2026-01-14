/*==============================================================================

   BULLET Controller[bullet.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef BULLET_H
#define BULLET_H

#include <DirectXMath.h>
#include "collision.h"

void Bullet_Initialize();
void Bullet_Finalize();
void Bullet_Update(double elapsedtime);
void Bullet_Draw();

void Bullet_Create(const DirectX::XMFLOAT3& position , const DirectX::XMFLOAT3& velocity);
void Bullet_Destroy(int index);


AABB Bullet_GetAABB(int index);

int Bullet_GetBulletCount();

const DirectX::XMFLOAT3& Bullet_GetPosition(int index);
#endif // !BULLET_H
